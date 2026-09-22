#pragma once
#include <unistd.h>                  // ::close
#include "server/noncopyable.hpp"

namespace server {

class Socket : public Noncopyable {
public:
    explicit Socket(int fd = -1) noexcept;   // 接管一个 fd(默认-1=空)
    ~Socket() noexcept;                      // 析构自动 close

    Socket(Socket&& other) noexcept;         // 移动构造(转移所有权)
    Socket& operator=(Socket&& other) noexcept; // 移动赋值(先关自己旧的)

    int  get()    const noexcept;   // 取 fd 给 read/write 用
    bool valid()  const noexcept;   // fd_ >= 0
    void close()         noexcept;  // 主动关并置 -1

private:
    int fd_ = -1;
};
Socket::Socket(int fd) noexcept : fd_(fd) {}
Socket::~Socket() noexcept{
    close();
}
Socket::Socket(Socket&& other) noexcept{
    fd_ = other.fd_;
    other.fd_ = -1;
}
Socket& Socket:: operator=(Socket&& other)noexcept{
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
}
int Socket::get() const noexcept{
    return fd_;
}
bool Socket::valid()  const noexcept{

    return fd_ >= 0;
}
void Socket::close() noexcept{
    if(valid()){
        ::close(fd_);
        fd_ = -1;
    }
}


}