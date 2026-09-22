#include <sys/epoll.h>
#include <cstring>         // memset
#include <sys/socket.h>    // socket/bind/listen/accept
#include <netinet/in.h>    // sockaddr_in
#include <arpa/inet.h>     // htons/htonl
#include <unistd.h>        // close/read/write
#include "server/log.hpp"
#include "server/errno.hpp"
#include "server/socket.hpp"


using namespace server;

int main(){

    auto& log = Logger::instance();
    log.set_level(Level::TRACE);
    log.log(Level::INFO, "server booting up");

    Socket lfd(::socket(AF_INET, SOCK_STREAM, 0));
    if(!lfd.valid()){
        ErrnoGuard eg;
        log.log(Level::ERROR, "socket() failed:", eg.message());
        return -1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);



    if((::bind(lfd.get(), reinterpret_cast<struct sockaddr*>(&server), sizeof(server))) < 0){
        ErrnoGuard eg;
        log.log(Level::ERROR, "bind() failed:", eg.message());
        return -1;
    }

    if(::listen(lfd.get(),256) < 0){
        ErrnoGuard eg;
        log.log(Level::ERROR, "listen() failed:", eg.message());
        return -1;
    }

    int epfd = ::epoll_create1(0);
    if (epfd < 0) {
        ErrnoGuard eg;
        log.log(Level::ERROR, "epoll_create() failed:", eg.message());
        return -1;
    }
    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = lfd.get();
    if(::epoll_ctl(epfd, EPOLL_CTL_ADD, lfd.get(), &ev) < 0){
        ErrnoGuard eg;
        log.log(Level::ERROR, "epoll_ctl() failed:", eg.message());
        return -1;
    }
    log.log(Level::INFO, "probe listening on 8888, waiting for a connection...");

    struct epoll_event evs[16];;

    int n = epoll_wait(epfd, evs, 16, -1);   // 阻塞直到有事件
    log.log(Level::INFO, "epoll_wait returned ", n, " event(s)");
    for (int i = 0; i < n; ++i)
    {
        int ready_fd  = evs[i].data.fd;     // 拷成普通 int
        int ready_ev  = evs[i].events;
        log.log(Level::INFO, "  ready fd=", ready_fd, " events=", ready_ev);
    }
        
    return 0;







}