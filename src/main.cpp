// 项目骨架入口:先用 Logger 演示冒烟,后续会把网络逻辑放进来
#include <iostream>
#include "server/log.hpp"

int main() {
    auto& log = server::Logger::instance();

    log.set_level(server::Level::TRACE);  // 打开全部等级,便于后续调试
    log.log(server::Level::INFO,  "server booting up");
    log.log(server::Level::DEBUG, "fd ", 3, " ready, peer sends ", 128, " bytes");
    log.log(server::Level::WARN,  "nonblocking not yet configured");
    log.log(server::Level::ERROR, "this line should appear");

    std::cout << "Stage 0 skeleton OK\n";
    return 0;
}