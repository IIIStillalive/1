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
#include <fcntl.h>

using namespace server;
// 定义自由函数 set_nonblocking
inline bool set_nonblocking(int fd) {
    int old_flags = ::fcntl(fd, F_GETFL, 0);
    if (old_flags < 0) return false;
    return ::fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) >= 0;
}

//连接柜
std::unordered_map<int,Socket> conns;


int main() {
    auto& log = Logger::instance();
    log.set_level(Level::TRACE);
    log.log(Level::INFO, "server booting up");

    Socket lfd(::socket(AF_INET, SOCK_STREAM, 0));
    if(!::set_nonblocking(lfd.get())){
        ErrnoGuard eg;
        log.log(Level::ERROR, "::fcntl() failed:", eg.message());
        return -1;
    }
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
    if(epfd < 0){
        ErrnoGuard eg;
        log.log(Level::ERROR, "::epoll_create1() failed:", eg.message());
        return -1; 
    }
    struct epoll_event ev{};
    ev.data.fd = lfd.get();
    ev.events = EPOLLIN | EPOLLET;
    if((::epoll_ctl(epfd, EPOLL_CTL_ADD, lfd.get(), &ev)) < 0){
        ErrnoGuard eg;
        log.log(Level::ERROR, "::epoll_ctl(EPOLL_CTL_ADD) failed:", eg.message());
        return -1; 
    }
    epoll_event ready[16];

    while(true){
        int n = ::epoll_wait(epfd, ready, 16, -1);
        for(int i = 0; i < n; i++){
            int fd = ready[i].data.fd;
            
            if(fd == lfd.get()){
                while(true){
                    int cfd = ::accept(lfd.get(), nullptr, nullptr);
                    if(cfd >= 0){
                        if(!::set_nonblocking(cfd)){
                            ErrnoGuard eg;
                            log.log(Level::ERROR, "::fcntl() failed:", eg.message());
                            return -1;
                        }
                        conns.emplace(cfd, Socket(cfd));
                        epoll_event cev{}; cev.events = EPOLLIN | EPOLLET; cev.data.fd = cfd;
                        ::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &cev);
                        log.log(Level::INFO, "accept new conn fd=", cfd);
                    }
                    else{
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        ErrnoGuard eg;
                        log.log(Level::ERROR, "::accept() failed:", eg.message());
                        return -1; 
                    }
                }
            } 
            else{
                char buf[4096];
                
                while(true){
                    memset(&buf, 0, sizeof(buf));
                    ssize_t r = ::read(fd, buf, sizeof(buf));
                    if(r > 0){
                        log.log(Level::INFO, "recv: ", r, "bytes");
                        ::write(fd, buf, static_cast<size_t>(r));
                    }
                    else if(r == 0){
                        if(::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) < 0){
                            ErrnoGuard eg;
                            log.log(Level::ERROR, "::epoll_ctl(EPOLL_CTL_DEL) failed:", eg.message());
                            return -1; 
                        }
                        conns.erase(fd);
                        log.log(Level::INFO, "closed fd=", fd);
                        break;
                    }
                    else if(errno == EAGAIN || errno == EWOULDBLOCK){
                        break;
                    }
                    else{
                        ErrnoGuard eg;
                        log.log(Level::ERROR, "::read() failed:", eg.message());
                        if(::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) < 0){
                            ErrnoGuard eg1;
                            log.log(Level::ERROR, "::epoll_ctl(EPOLL_CTL_DEL) failed:", eg1.message());
                            return -1; 
                        }
                        conns.erase(fd);
                        return -1; 
                    }
                }

            }
        }
    }
    return 0;                                             
}