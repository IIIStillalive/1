#pragma once
#include <cerrno>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unistd.h>                 // ::read/::write/::close
#include <sys/eventfd.h>            // ::eventfd
#include "server/channel.hpp"
#include "server/socket.hpp"
#include "server/thread_pool.hpp"

namespace server {

// 业务层：一条连接 = 资源(Socket) + 事件分发(Channel) + 本连接的处理逻辑。
// 慢任务跨线程：worker 只算、把结果写进队列并通过 eventfd 唤醒事件线程；
// 事件线程被唤醒后取队列写回 socket（socket / epoll 状态只允许事件线程碰）。
class TcpConnection {
public:
    TcpConnection(int fd, std::shared_ptr<ThreadPool> pool);
    ~TcpConnection();

    int      fd()      const noexcept { return sock_.get(); }  // data socket
    int      wakeFd()  const noexcept { return wake_fd_; }     // eventfd
    uint32_t events()  const noexcept { return channel_.events(); }

    // EventLoop 分发入口：revents + 触发事件的那个 fd（data 或 wake）
    void dispatch(uint32_t revents, int fd);

    void requestRemoval() noexcept { remove_ = true; }
    bool removed() const noexcept { return remove_; }

private:
    void handleData();              // 事件线程：读 socket → 交给 worker 慢算
    void handleWake();              // 事件线程：eventfd 可读 → 把结果写回 socket
    void processAsync(std::string data);      // worker 线程：纯计算，不碰 fd

    Socket    sock_;                // 拥有 data fd（事件线程独占）
    Channel   channel_;             // data fd 的分发器
    int       wake_fd_ = -1;        // eventfd：worker → 事件线程 唤醒
    std::shared_ptr<ThreadPool> pool_;

    std::mutex           out_mtx_;  // 保护结果队列（worker 写 / 事件线程取）
    std::deque<std::string> out_queue_;

    bool remove_ = false;
};

inline TcpConnection::TcpConnection(int fd, std::shared_ptr<ThreadPool> pool)
    : sock_(fd), channel_(fd), pool_(std::move(pool)) {
    wake_fd_ = ::eventfd(0, EFD_NONBLOCK);     // 0 初值 + 非阻塞
    channel_.enableReading();
    channel_.setReadCallback([this] { handleData(); });
}

inline TcpConnection::~TcpConnection() {
    if (wake_fd_ >= 0) ::close(wake_fd_);
}

inline void TcpConnection::dispatch(uint32_t revents, int fd) {
    if (fd == wake_fd_) handleWake();          // eventfd → 取结果写回
    else                channel_.handleEvent(revents);  // data fd → 读 → 慢算
}

// —— 事件线程：读数据，投给 worker 慢算 ——
inline void TcpConnection::handleData() {
    char buf[4096];
    while (true) {                               // ET：读到 EAGAIN 为止
        ssize_t r = ::read(sock_.get(), buf, sizeof(buf));
        if (r > 0) {
            if (pool_) {
                std::string data(buf, static_cast<size_t>(r));
                pool_->submit([this, data = std::move(data)]() mutable {
                    processAsync(std::move(data));          // worker 线程
                });
            }
        } else if (r == 0) {                     // 对端关闭
            requestRemoval();
            return;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;                              // 读空，等下次
        } else {                                 // 真错误
            requestRemoval();
            return;
        }
    }
}

// —— worker 线程：纯计算（模拟耗时），结果入队并唤醒事件线程 ——
inline void TcpConnection::processAsync(std::string data) {
    // 模拟耗时计算：按数据长度做忙转（把首尾调换之类，只是制造耗时）
    volatile unsigned long x = 0;
    for (size_t i = 0; i < 2000000 + data.size() * 1; ++i) x += i;   // 微秒级忙循环
    std::string out = std::move(data);           // 假装"算完的结果"

    {                                            // 入队（worker 写）
        std::lock_guard<std::mutex> lk(out_mtx_);
        out_queue_.push_back(std::move(out));
    }
    uint64_t one = 1;
    ::write(wake_fd_, &one, sizeof(one));        // 唤醒事件线程（只碰 eventfd）
}

// —— 事件线程：eventfd 可读 → 取空队列，写回 socket ——
inline void TcpConnection::handleWake() {
    uint64_t cnt;
    while (::read(wake_fd_, &cnt, sizeof(cnt)) > 0) {}  // 读计数到 EAGAIN，清零

    std::deque<std::string> batch;
    {
        std::lock_guard<std::mutex> lk(out_mtx_);
        batch.swap(out_queue_);                  // 取空全部（可能多个 worker 完成）
    }
    for (auto& s : batch)                        // 写回 socket（事件线程独占）
        if (::write(sock_.get(), s.data(), s.size()) < 0) break;
}

}