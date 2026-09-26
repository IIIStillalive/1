#pragma once
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include "server/channel.hpp"
#include "server/tcp_connection.hpp"

namespace server {

// 只负责事件就绪 → 分发到对应处理单元。
// 用 epoll 的 ev.data.ptr 直接记对象指针，分发时免查表：
//   监听 fd → Channel；业务连接 → TcpConnection（它的 data fd 与 wake fd 都指向同一 conn）。
class EventLoop {
public:
    EventLoop();                                          // epoll_create1，失败抛异常
    ~EventLoop();

    bool addListenChannel(std::unique_ptr<Channel> ch);   // 拥有监听 Channel
    bool addConnection(std::unique_ptr<TcpConnection> conn); // 拥有一条业务连接
    void run();                                           // while(true) epoll_wait + 分发
private:
    int epfd_ = -1;
    std::unique_ptr<Channel> listen_;
    std::unordered_map<int, std::unique_ptr<TcpConnection>> conns_;
};

inline EventLoop::EventLoop() : epfd_(::epoll_create1(0)) {
    if (epfd_ < 0)
        throw std::runtime_error(std::string("epoll_create1 failed: ") + std::strerror(errno));
}
inline EventLoop::~EventLoop() { if (epfd_ >= 0) ::close(epfd_); }

inline bool EventLoop::addListenChannel(std::unique_ptr<Channel> ch) {
    int fd = ch->fd();
    struct epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = ch.get();                              // 监听对象指针
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0) return false;
    listen_ = std::move(ch);
    return true;
}

inline bool EventLoop::addConnection(std::unique_ptr<TcpConnection> conn) {
    int data_fd = conn->fd();
    int wake_fd = conn->wakeFd();
    TcpConnection* ptr = conn.get();

    struct epoll_event ev1{};                            // data fd
    ev1.events = conn->events();                          // EPOLLIN | EPOLLET
    ev1.data.ptr = ptr;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, data_fd, &ev1) < 0) return false;

    struct epoll_event ev2{};                            // eventfd（唤醒）
    ev2.events = EPOLLIN;                                 // LT：计数非 0 就通知，handleWake 读到 EAGAIN
    ev2.data.ptr = ptr;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, wake_fd, &ev2) < 0) {
        ::epoll_ctl(epfd_, EPOLL_CTL_DEL, data_fd, nullptr);
        return false;
    }

    conns_.emplace(data_fd, std::move(conn));            // 拥有它
    return true;
}

inline void EventLoop::run() {
    struct epoll_event ready[16];
    while (true) {
        int n = ::epoll_wait(epfd_, ready, 16, -1);
        if (n < 0) {
            if (errno == EINTR) continue;        // 被信号打断，重试
            throw std::runtime_error("epoll_wait failed");
        }
        for (int i = 0; i < n; ++i) {
            void* p = ready[i].data.ptr;

            if (listen_ && p == listen_.get()) {           // 监听 fd
                listen_->handleEvent(ready[i].events);
                continue;
            }

            auto* conn = static_cast<TcpConnection*>(p);   // 业务连接（data 或 wake 触发均指向它）
            conn->dispatch(ready[i].events, ready[i].data.fd);
            if (conn->removed()) {
                // 两个 fd 都从 epoll 摘掉，再析构连接
                ::epoll_ctl(epfd_, EPOLL_CTL_DEL, conn->fd(),      nullptr);
                ::epoll_ctl(epfd_, EPOLL_CTL_DEL, conn->wakeFd(),  nullptr);
                conns_.erase(conn->fd());                // 析构 TcpConnection → Socket/Channel/eventfd 随之释放
            }
        }
    }
}

}