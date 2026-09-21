# 学习计划：低延迟高性能网络服务器

> 每次只攻克一个核心概念。完成一项勾选一项。卡住的问题记在 notes.md。

## 里程碑

### Stage 0 — 环境与骨架
- [x] WSL2 Ubuntu 24.04 + g++ 13 + gdb + make 就绪
- [x] 安装 cmake + valgrind（源已切 USTC，以 root 安装，避开 sudo 密码卡死）
- [x] 项目结构 + CMake 能 build + run 烟测（输出 Stage 0 skeleton OK）
- [x] 第一课 RAII：NonCopyable 基类（含 const 正确性、namespace server，已验证禁止拷贝）
- [x] 第一课 RAII：ErrnoGuard 错误处理封装（含 errno 是宏、恢复契约=构造瞬间值）
- [x] 第二课：模块化日志（单例、折叠表达式、两阶段查找、声明顺序；已接入 main 构建运行）
- [ ] git init + 首次提交

### Stage 1 — 阻塞回显服务器
- [ ] socket/bind/listen/accept
- [ ] 回显 read/write
- [ ] GDB 实战：断点、backtrace

### Stage 2 — 非阻塞 + epoll
- [ ] O_NONBLOCK
- [ ] epoll LT 实现
- [ ] epoll ET 实现 + 与 LT 对比
- [ ] EAGAIN 处理

### Stage 3 — 单线程 Reactor
- [ ] 事件循环
- [ ] fd → handler 注册表
- [ ] 回调分发
- [ ] 连接生命周期管理

### Stage 4 — 多线程线程池
- [ ] 线程安全任务队列
- [ ] mutex + condition_variable
- [ ] 固定线程池
- [ ] 事件循环 → 工作线程分发

### Stage 5 — 缓冲与协议
- [ ] ByteBuffer（读写双缓冲）
- [ ] 应用层协议 + 粘包/拆包
- [ ] （可选）对象池
- [ ] 内存移动语义 / ring buffer

### Stage 6 — 性能优化与诊断
- [ ] TCP_NODELAY / SO_REUSEPORT
- [ ] 定时器（最小堆）
- [ ] 零拷贝 splice/sendfile
- [ ] valgrind 内存检查
- [ ] perf 定位热点 + 压测基准

## 技术栈
- 语言：C++20
- OS：Linux（WSL2 Ubuntu 24.04）
- 网络：epoll（I/O 多路复用）
- 架构：Reactor + 线程池
- 构建：CMake
- 调试：gdb / valgrind / perf