#pragma once
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include "server/channel.hpp"

namespace server {

// 只认识通用 Channel（fd→分发）；完全不认识任何业务连接类型。
class EventLoop {
public:
    EventLoop();                                          // epoll_create1，失败抛异常
    ~EventLoop();

    bool add(std::unique_ptr<Channel> ch);                // 拥有一个 Channel（含监听与连接）
    void run();                                           // while(true) epoll_wait + 分发
private:
    int epfd_ = -1;
    std::unordered_map<int, std::unique_ptr<Channel>> channels_;
};

EventLoop::EventLoop() : epfd_(::epoll_create1(0)) {
    if (epfd_ < 0)
        throw std::runtime_error(std::string("epoll_create1 failed: ") + std::strerror(errno));
}
EventLoop::~EventLoop() { if (epfd_ >= 0) ::close(epfd_); }

bool EventLoop::add(std::unique_ptr<Channel> ch) {
    int fd = ch->fd();
    struct epoll_event ev{};
    ev.events = ch->events();
    ev.data.fd = fd;
    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0) return false;
    channels_.emplace(fd, std::move(ch));   // 拥有它
    return true;
}

void EventLoop::run() {
    struct epoll_event ready[16];
    while (true) {
        int n = ::epoll_wait(epfd_, ready, 16, -1);
        if (n < 0) {
            if (errno == EINTR) continue;        // 被信号打断，重试
            throw std::runtime_error("epoll_wait failed");
        }
        for (int i = 0; i < n; ++i) {
            int fd = ready[i].data.fd;
            auto it = channels_.find(fd);
            if (it == channels_.end()) continue;                 // 残留事件，忽略
            it->second->handleEvent(ready[i].events);            // 分发（栈帧内可安全执行）

            // —— 事后删除：栈帧已弹出后才删 ——
            if (it->second->removed()) {
                ::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
                channels_.erase(it);                             // 析构 Channel → 资源随回调闭包释放
            }
        }
    }
}

}