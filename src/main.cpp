// Stage 4: Reactor — EventLoop 只做分发；业务在 TcpConnection 里；监听 fd 用 Channel
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
#include "server/tcp_connection.hpp"
#include "server/thread_pool.hpp"

using namespace server;

inline bool set_nonblocking(int fd) {
    int old_flags = ::fcntl(fd, F_GETFL, 0);
    if (old_flags < 0) return false;
    return ::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) >= 0;
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
    // 共享线程池：所有连接共用，慢任务丢这里，事件线程只做非阻塞分发
    auto pool = std::make_shared<ThreadPool>(4);
    int lfd_val = lfd.get();

    // 监听 fd → 一个 Channel；accept 到 EAGAIN，把每个新连接包成 TcpConnection 交给 loop
    auto listen_ch = std::make_unique<Channel>(lfd_val);
    listen_ch->setReadCallback([&loop, lfd_val, pool]() {
        while (true) {
            int cfd = ::accept(lfd_val, nullptr, nullptr);
            if (cfd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // 队列空，等下次
                ErrnoGuard eg;
                Logger::instance().log(Level::ERROR, "accept() failed:", eg.message());
                return;
            }
            ::set_nonblocking(cfd);              // 连接 fd 也要非阻塞
            loop.addConnection(std::make_unique<TcpConnection>(cfd, pool));
        }
    });
    listen_ch->enableReading();
    if (!loop.addListenChannel(std::move(listen_ch))) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "addListenChannel() failed:", eg.message());
        return -1;
    }

    loop.run();
    return 0;
}