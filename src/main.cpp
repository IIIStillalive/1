// Stage 1: 阻塞回环服务器(Socket RAII 版)
#include <cstring>         // memset
#include <sys/socket.h>    // socket/bind/listen/accept
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // close/read/write
#include "server/log.hpp"
#include "server/errno.hpp"
#include "server/socket.hpp"
using namespace server;

int main() {
    auto& log = Logger::instance();
    log.set_level(Level::TRACE);
    log.log(Level::INFO, "server booting up");

    Socket lfd(::socket(AF_INET, SOCK_STREAM, 0));
    if (!lfd.valid()) {                                   // ① valid()
        ErrnoGuard eg;
        log.log(Level::ERROR, "socket() failed:", eg.message());
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family      = AF_INET;
    server.sin_port        = htons(8888);
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (::bind(lfd.get(), reinterpret_cast<struct sockaddr*>(&server), sizeof(server)) < 0) {  // ② get()
        ErrnoGuard eg;
        log.log(Level::ERROR, "bind() failed:", eg.message());
        return -1;
    }

    if (::listen(lfd.get(), 256) < 0) {                   // ③ get()
        ErrnoGuard eg;
        log.log(Level::ERROR, "listen() failed:", eg.message());
        return -1;
    }
    log.log(Level::INFO, "listening on 127.0.0.1:8888");

    Socket cfd(::accept(lfd.get(), nullptr, nullptr));    // ④ get()
    if (!cfd.valid()) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "accept() failed:", eg.message());
        return -1;
    }

    char buf[4096];
    for (;;) {
        ssize_t n = ::read(cfd.get(), buf, sizeof(buf));  // ⑤ 读用 get()
        if (n == 0) {
            log.log(Level::INFO, "peer closed the connection");
            break;
        }
        if (n < 0) {
            ErrnoGuard eg;
            log.log(Level::ERROR, "read() failed:", eg.message());
            break;
        }
        log.log(Level::INFO, "recv ", n, " bytes, echoing back");
        ::write(cfd.get(), buf, static_cast<size_t>(n));  // ⑥ 写用 get()
    }
    return 0;                                             // ⑦ Socket 析构自动 close
}