# 低延迟高性能网络服务器（学习项目）

基于 **C++20 + Linux/epoll** 从零手写的高性能网络服务器。
采用 **Reactor 模式 + 线程池** 架构，边做边学。

## 目标

- 掌握现代 C++（RAII、智能指针、移动语义、多线程同步）
- 掌握 Linux 底层网络（非阻塞 I/O、epoll LT/ET、零拷贝）
- 最终产物：一个生产级可扩展的高性能事件驱动服务器

## 学习路线（见 task_plan.md）

| 阶段 | 主题 | 核心概念 |
|------|------|----------|
| 0 | 环境与骨架 | CMake、RAII、工程布局 |
| 1 | 阻塞 Socket | socket/bind/accept、字节序 |
| 2 | 非阻塞 + epoll | O_NONBLOCK、LT/ET、EAGAIN |
| 3 | 单线程 Reactor | 事件循环、回调分发 |
| 4 | 多线程线程池 | mutex、condition_variable、任务队列 |
| 5 | 缓冲与协议 | ByteBuffer、粘包、对象池 |
| 6 | 性能优化 | TCP_NODELAY、定时器、零拷贝、perf/valgrind |

## 构建

```bash
# 在 WSL2 Ubuntu 内
cd /mnt/f/C++项目/低延迟高性能服务器
cmake -S . -B build && cmake --build build
./build/server
```