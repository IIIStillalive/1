# 学习进度 NOTES

> 记录已完成的关卡与关键认知，方便下次续接。路线图见 task_plan.md。

## 已掌握（时间线）
- **Stage 0 环境/骨架**：WSL Ubuntu 24.04、g++13、cmake 3.28、valgrind、源切 USTC。CMake 两步流程（configure / build）。
- **第一课 RAII**：
  - `Noncopyable`：禁拷贝基类（`= delete` 拷贝构造/赋值）+ `namespace server`。
  - `ErrnoGuard`：构造捕获 errno、析构恢复；**errno 是宏不是变量，`::errno` 会报错**；"恢复"契约 = 恢复到自己接管时的值。
- **第二课 Logger**：单例、折叠表达式 `(os<<...<<args)`、两阶段查找（非依赖名在模板定义时查找 → 前置声明）、声明顺序（enum → level_str → class）。

## 关键坑（复习用）
- `#` 是预处理指令不是注释（注释是 `//`）。
- sudo 在非交互 shell 会**静默等密码卡死** → 用 `wsl -u root` 免密。
- 两层 apt 并发 = dpkg 锁竞争。
- errno/两阶段查找/声明顺序 —— 见上。

## Git 状态
- 远端 `origin` = github.com/IIIStillalive/1，分支 `main`，已同步（`2f95f02..f20bc38`）。
- 用户本人负责以后所有 cmake/git 操作。

## 下次：Stage 1 — 阻塞 Socket 回显服务器
概念预览：`socket/bind/listen/accept/read/write`、字节序（htons/htonl）、`AF_INET`、sockaddr_in、listen 排队、accept 返回新 fd。
会用到：Logger + ErrnoGuard。