// Stage 4 smoke test: 独立验证 ThreadPool —— 不参与 server 构建
// 验证：1) 并行执行不超线程数 2) 任务内再 submit 不死锁 3) 析构正常收尾
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include "server/thread_pool.hpp"

using namespace server;
using namespace std::chrono_literals;

static std::atomic<int> running_peak{0};
static std::atomic<int> done{0};

static void busy_work() {
    auto t0 = std::chrono::steady_clock::now();
    volatile unsigned long x = 0;
    while (std::chrono::steady_clock::now() - t0 < 5ms) x += 1;
}

int main() {
    const int num_workers = 4;
    const int num_tasks   = 16;
    {
        ThreadPool pool(num_workers);

        // 验证 1+3:提交一堆忙任务,观测并行度,计数完成
        for (int i = 0; i < num_tasks; ++i) {
            pool.submit([i] {
                int now = running_peak.fetch_add(1) + 1;   // 当前同时执行的任务数
                if (now > num_workers)
                    std::fprintf(stderr, "FAIL: parallelism %d > %d\n", now, num_workers);
                busy_work();
                running_peak.fetch_sub(1);
                std::printf("task %2d done on thread %u\n", i,
                            static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
                done.fetch_add(1);
            });
        }

        // 验证 2:任务内再 submit —— 锁外执行若对不对,这里会死锁
        for (int i = 0; i < num_workers; ++i) {            // 让每个 worker 都有机会接到这样的任务
            pool.submit([&pool, i] {
                std::printf("[nest-%d] outer on thread %u, submitting inner\n",
                            i, static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
                pool.submit([i] {                          // worker 线程执行到这里,会去抢锁
                    std::printf("[nest-%d] inner done on thread %u\n",
                                i, static_cast<unsigned>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
                });
            });
        }
    }                                                       // 离开作用域 → pool 析构,应正常收尾

    std::printf("all tasks done, total %d\n", done.load());
    return done.load() == num_tasks ? 0 : 1;               // 全部完成才算通过
}