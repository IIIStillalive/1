#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>

namespace server{

class ThreadPool{

public:
    explicit ThreadPool(size_t n);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void submit(std::function<void()> task);
    
private:

    void workloop();

    std::queue<std::function<void()>> task_;
    std::vector<std::thread> workers_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop = false;

};


void ThreadPool::workloop(){
    //
    std::function<void()> task;
    while(true){
        {        
            std::unique_lock<std::mutex> lock(mtx_); //先上锁
            cv_.wait(lock, [this](){ return this->stop || !this->task_.empty();}); //等待唤醒
            if(stop && task_.empty()) return;
            task = std::move(this->task_.front()); 
            task_.pop();
        }
        task(); //先拷贝后删除,再锁外执行，多线程安全
    }
}
ThreadPool::ThreadPool(size_t n){
    for(size_t i = 0; i < n; ++i){
        workers_.emplace_back([this](){ workloop();}); //初始化线程池，关联线程处理函数
    }
}

ThreadPool::~ThreadPool(){
    {

        std::lock_guard<std::mutex> lock(this->mtx_); 
        this->stop = true; //操作共享资源要上锁
    }
    
    cv_.notify_all(); //先设置true再唤醒

    for(auto& it : workers_) it.join(); // 等待进程执行完回收

}

//为生产者设计的接口
void ThreadPool::submit(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(mtx_);
        task_.emplace(task);
    }
    cv_.notify_one();
}


}