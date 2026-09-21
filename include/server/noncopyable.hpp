#pragma once
namespace server
{
class Noncopyable
{
public:
    Noncopyable() = default;
    Noncopyable( const Noncopyable& ) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
    ~Noncopyable() = default;
};

}