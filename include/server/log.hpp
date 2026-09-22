#pragma once


#include <sstream>
#include <utility>
#include "server/noncopyable.hpp"
#include <iostream>
namespace server
{

enum class Level{ TRACE, DEBUG, INFO, WARN, ERROR};

// 一对自由函数: 等级->字符串
const char* level_str(Level lv)
{
    switch (lv) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
        default:           return "UNKNOWN";
    }
}

class Logger: public Noncopyable
{
public:
    static Logger& instance();
    void set_level(Level lv);
    template<typename... Args>
    void log(Level lv, Args&&... args)
    {
        std::ostringstream os;
        os << level_str(lv) << ":";
        (os << ... << std::forward<Args>(args));
        do_log(lv, os.str());

    }
private:
    Logger() = default; //私有只能由instance 创建
    void do_log(Level lv, const std::string& msg);
    Level level_ = Level::INFO;
};


Logger& Logger::instance()
{
    static Logger inst;
    return inst;
}

void Logger::do_log(Level lv, const std::string& msg)
{
    std::cout << msg << '\n'; 
    (void)lv;
}

void Logger::set_level(Level lv)
{
    this->level_ = lv;
}

}