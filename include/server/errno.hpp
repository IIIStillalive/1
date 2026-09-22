#pragma once

#include <cerrno>
#include <cstring>
#include <string>


namespace server
{
class ErrnoGuard
{
public:
    explicit ErrnoGuard() noexcept; 
    ~ErrnoGuard() noexcept;

    int code() const noexcept;
    std::string message() const;
    bool ok() const noexcept;

    ErrnoGuard(const ErrnoGuard&) = delete;
    ErrnoGuard& operator=(const ErrnoGuard& ) = delete;
private:
    int saved_;
    int prev_;
}; 


ErrnoGuard::ErrnoGuard() noexcept : saved_{errno}, prev_{errno} {}
ErrnoGuard::~ErrnoGuard() noexcept
{
    errno = prev_;   // errno 是宏,展开为 lvalue,去掉 ::
}
int ErrnoGuard::code() const noexcept
{
    return this->saved_;
}
std::string ErrnoGuard:: message() const
{
    return std::strerror(saved_);
}
bool ErrnoGuard::ok() const noexcept
{
    return saved_ == 0;
}
}





