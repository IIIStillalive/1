#pragma once
#include <functional>
#include <sys/epoll.h>

namespace server {

// Channel = "一个 fd + 当它就绪时调谁"。只做事件分发，不认识任何业务。
class Channel {
public:
    using Callback = std::function<void()>;

    explicit Channel(int fd) noexcept : fd_(fd), events_(0) {}
    ~Channel() = default;

    void enableReading() noexcept { events_ = EPOLLIN | EPOLLET; }

    void setReadCallback (Callback cb) { readCallback_  = std::move(cb); }
    void setWriteCallback(Callback cb) { writeCallback_ = std::move(cb); }

    int      fd()     const noexcept { return fd_; }
    uint32_t events() const noexcept { return events_; }

    // 业务想关掉自己时，只标记；由 EventLoop 在分发之后统一 remove——不在此刻删 this
    void requestRemoval()   noexcept { remove_ = true; }
    bool removed()    const noexcept { return remove_; }

    // EventLoop 就绪后调这里：把 revents 分发到对应回调
    void handleEvent(uint32_t revents) {
        if (revents & (EPOLLIN | EPOLLERR | EPOLLHUP)) {   // 读相关（含对端关闭）
            if (readCallback_) readCallback_();
        }
        if (revents & EPOLLOUT) {
            if (writeCallback_) writeCallback_();
        }
    }

private:
    int      fd_;
    uint32_t events_;
    bool     remove_ = false;
    Callback readCallback_, writeCallback_;
};

}