// Stage 3: Reactor — EventLoop 只认 Channel；连接业务挂回调闭包
#include <cstring>         // memset
#include <sys/socket.h>    // socket/bind/listen/accept
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // close/read/write
#include <fcntl.h>         // fcntl
#include <memory>          // make_unique
#include "server/log.hpp"
#include "server/errno.hpp"
#include "server/socket.hpp"
#include "server/channel.hpp"
#include "server/event_loop.hpp"

using namespace server;

inline bool set_nonblocking(int fd) {
    int old_flags = ::fcntl(fd, F_GETFL, 0);
    if (old_flags < 0) return false;
    return ::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) >= 0;
}

// 把一个连接 fd 包装成 Channel：回调闭包持有 Socket，EOF/出错时标记移除
static std::unique_ptr<Channel> makeEchoChannel(int cfd) {
    auto ch = std::make_unique<Channel>(cfd);
    Channel* raw = ch.get();                       // 回指自身，仅用于 requestRemoval（标志位，非删除）
    ch->enableReading();
    ch->setReadCallback([sock = std::make_shared<Socket>(cfd), raw]() mutable {
        char buf[4096];
        while (true) {                              // ET：读到 EAGAIN 为止
            ssize_t r = ::read(sock->get(), buf, sizeof(buf));
            if (r > 0) {
                ::write(sock->get(), buf, static_cast<size_t>(r));   // 回显
            } else if (r == 0) {
                raw->requestRemoval();              // 对端关闭：只标记
                return;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;                             // 读空，等下次
            } else {
                raw->requestRemoval();              // 真错误：只标记
                return;
            }
        }
    });
    return ch;
}

int main() {
    auto& log = Logger::instance();
    log.set_level(Level::TRACE);
    log.log(Level::INFO, "server booting up");

    Socket lfd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!lfd.valid()) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "socket() failed:", eg.message());
        return -1;
    }
    if (!::set_nonblocking(lfd.get())) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "fcntl() failed:", eg.message());
        return -1;
    }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family      = AF_INET;
    srv.sin_port        = htons(8888);
    srv.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(lfd.get(), reinterpret_cast<struct sockaddr*>(&srv), sizeof(srv)) < 0) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "bind() failed:", eg.message());
        return -1;
    }
    if (::listen(lfd.get(), 256) < 0) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "listen() failed:", eg.message());
        return -1;
    }
    log.log(Level::INFO, "listening on 127.0.0.1:8888");

    EventLoop loop;
    int lfd_val = lfd.get();

    // 监听 fd → 一个 Channel；accept 到 EAGAIN，把每个新连接注册进 loop
    auto listen_ch = std::make_unique<Channel>(lfd_val);
    listen_ch->setReadCallback([&loop, lfd_val]() {
        while (true) {
            int cfd = ::accept(lfd_val, nullptr, nullptr);
            if (cfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 队列空，等下次
                ErrnoGuard eg;
                Logger::instance().log(Level::ERROR, "accept() failed:", eg.message());
                return;
            }
            ::set_nonblocking(cfd);              // 连接 fd 也要非阻塞
            loop.add(makeEchoChannel(cfd));
        }
    });
    listen_ch->enableReading();
    if (!loop.add(std::move(listen_ch))) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "addListener() failed:", eg.message());
        return -1;
    }

    loop.run();
    return 0;
}