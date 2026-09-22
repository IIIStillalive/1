# 学习进度 NOTES

> 记录已完成的关卡与关键认知，方便下次续接。路线图见 task_plan.md。

## 已掌握（时间线）
- **Stage 0 环境/骨架**：WSL Ubuntu 24.04、g++13、cmake 3.28、valgrind、源切 USTC。CMake 两步流程（configure / build）。
- **第一课 RAII**：
  - `Noncopyable`：禁拷贝基类（`= delete` 拷贝构造/赋值）+ `namespace server`。
  - `ErrnoGuard`：构造捕获 errno、析构恢复；**errno 是宏不是变量，`::errno` 会报错**；"恢复"契约 = 恢复到自己接管时的值。
- **第二课 Logger**：单例、折叠表达式 `(os<<...<<args)`、两阶段查找（非依赖名在模板定义时查找 → 前置声明）、声明顺序（enum → level_str → class）。
- **Stage 1 阻塞回环服务器**：Socket RAII（move ctor 不能 close, move 赋值要先 close 旧的）、sockaddr_in / htons / INADDR_LOOPBACK、socket→bind→listen→accept→read/write。
- **Stage 2 epoll 事件循环**：epoll_create1(0)（不带 size 参数,EINVAL 坑）、epoll_ctl ADD/DEL、epoll_wait；LT vs ET。
  - **ET 核心认知**：ET 只在"缓冲从空变有"报一次 → 必须读/接到 **EAGAIN** 才够（否则剩的数据永远不再通知）。
  - 非阻塞：fcntl F_GETFL/F_SETFL 加 O_NONBLOCK。
  - **最典型的 bug**：`if(set_nonblocking(cfd))` 把"成功(true)"当错误分支 → 打印 `fcntl() failed:Success`(errno=0 就是 strerror(0))。判错用 `!`。
  - 连接表 `unordered_map<int,Socket>` 保 fd 生命周期;close 时 epoll_ctl DEL + erase。

## 关键坑（复习用）
- `#` 是预处理指令不是注释（注释是 `//`）。
- sudo 在非交互 shell 会**静默等密码卡死** → 用 `wsl -u root` 免密。
- 两层 apt 并发 = dpkg 锁竞争。
- errno/两阶段查找/声明顺序 —— 见上。

## Git 状态
- 远端 `origin` = github.com/IIIStillalive/1，分支 `main`，已同步（`2f95f02..f20bc38`）。
- 用户本人负责以后所有 cmake/git 操作。

## 下次：Stage 3 — 把事件循环抽成 Reactor / EventLoop 类
概念预览：把 main() 里裸的 `while(true)` 事件循环 + fd 分发提炼成类。会话涉及 C++ 概念：**回调分发**（std::function / 抽象基类回调）、连接生命周期归 EventLoop 管、监听 fd 与连接 fd 的统一事件回调。之后 Stage 4 进阶 thread pool。
会用到：显式确认当前 ET 版已 verify（10 万字节 python 一次性发送全回显、多 nc 并发 accept 不漏连）。