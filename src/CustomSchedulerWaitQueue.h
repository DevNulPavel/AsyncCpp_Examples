#pragma once

#include <atomic>
#include <thread>
#include <queue>
#include <async++.h>

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
class CustomSchedulerWaitQueue {
public:
    CustomSchedulerWaitQueue();
    ~CustomSchedulerWaitQueue();
    
    void waitAndPerformAllTasks();
    void stopWaiting();
    bool isStopRequested() const;
    
    void schedule(async::task_run_handle t);

private:
    std::atomic_bool _stopWaiting;
    std::mutex _mutex;
    std::condition_variable _condVar;
    std::queue<async::task_run_handle> _queue;
};
