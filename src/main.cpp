// Stage 1: 阻塞回环服务器(Socket RAII 版)
#include <cstring>         // memset
#include <sys/socket.h>    // socket/bind/listen/accept
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // close/read/write
#include "server/log.hpp"
#include "server/errno.hpp"
#include "server/socket.hpp"
#include <sys/epoll.h>
#include <unordered_map>

using namespace server;

//连接柜
std::unordered_map<int,Socket> conns;
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

    int epfd = ::epoll_create1(0);
    struct epoll_event ev{};
    ev.data.fd = lfd.get();
    ev.events = EPOLLIN;
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, lfd.get(), &ev);

    epoll_event ready[16];

    while(true){
        int n = ::epoll_wait(epfd, ready, 16, -1);
        for(int i = 0; i < n; i++){
            int fd = ready[i].data.fd;
            
            if(fd == lfd.get()){
                int cfd = ::accept(lfd.get(), nullptr, nullptr);
                if(cfd >= 0){
                    conns.emplace(cfd, Socket(cfd));
                    epoll_event cev{}; cev.events = EPOLLIN; cev.data.fd = cfd;
                    ::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev);
                    log.log(Level::INFO, "accept new conn fd=", cfd);
                }
            } 
            else{
                char buf[4096];
                memset(&buf, 0, sizeof(buf));
                ssize_t r = ::read(fd, buf, sizeof(buf));
                if(r > 0){
                    ::write(fd, buf, static_cast<size_t>(r));
                }
                else{
                    ::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    conns.erase(fd);
                    log.log(Level::INFO, "closed fd=", fd);
                }
            }
        }
    }
    return 0;                                             
}