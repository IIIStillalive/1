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
- **Stage 3 Reactor**：把 epoll 循环抽象成 EventLoop，业务用回调注入。
  - **Channel**：薄分发工具，持有 `int fd_` + `std::function` 回调 + `handleEvent(revents)`，不拥有 fd、不认识业务。
  - **核心边界（关键认知）**：EventLoop 只认 `Channel`，绝不认识业务连接类型；业务（echo）挂 Channel 的 `readCallback` 闭包。
  - **资源用 unique_ptr 拥有**：`EventLoop` 持 `unordered_map<fd, unique_ptr<Channel>>`，不设裸指针借用表；连接资源（Socket）move 进回调闭包，随 Channel 析构释放。
  - **杜绝自杀删除**：业务想关自己只 `requestRemoval()`（置标志），EventLoop 在 `handleEvent` 栈帧弹出后查 `removed()` 再 `channels_.erase`。
  - **std::function 要求可拷**：捕获 move-only（如 `unique_ptr`）会编译炸 → 改用 `shared_ptr` 捕获（可拷，单一存活持有）。
  - **继承 vs 回调注入**：行为多态用 std::function（运行期绑定、可换）；继承把多态锁进 vtable，适合「是」关系与共享实现，不适合承载可变业务。

## 关键坑（复习用）
- `#` 是预处理指令不是注释（注释是 `//`）。
- sudo 在非交互 shell 会**静默等密码卡死** → 用 `wsl -u root` 免密。
- 两层 apt 并发 = dpkg 锁竞争。
- errno/两阶段查找/声明顺序 —— 见上。

## Git 状态
- 远端 `origin` = github.com/IIIStillalive/1，分支 `main`，已同步（`2f95f02..f20bc38`）。
- 用户本人负责以后所有 cmake/git 操作。

## 下次：Stage 4 — 多线程线程池
动机预览：现在单线程 Reactor 一个线程串行处理 accept/read/echo。若 echo 逻辑是耗时计算或阻塞 I/O，整个循环卡住（一个慢连接拖垮全部）。阶段引入：**线程安全任务队列（mutex + condition_variable）+ 固定线程池 + 事件循环把"耗时任务"投递到工作线程**。
衔接点：Stage 3 的 EventLoop 只负责 epoll 就绪唤醒，不碰计算——这正好成为"事件线程只做非阻塞、耗时的丢给池子"的切分基础。