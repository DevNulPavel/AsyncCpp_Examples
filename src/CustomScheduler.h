#pragma once

#include <atomic>
#include <thread>
#include <queue>
#include <async++.h>

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
class CustomScheduler {
public:
    CustomScheduler();
    ~CustomScheduler();

    size_t getQueueSize();
    bool taskIsInWork() const;
    
    void schedule(async::task_run_handle t);

private:
    std::atomic_bool _stopThread;
    std::mutex _mutex;
    std::condition_variable _condVar;
    std::queue<async::task_run_handle> _queue;
    std::atomic_bool _taskIsInWork;
    std::condition_variable _condVarExit;
    bool _threadExitSuccess;
};

std::shared_ptr<CustomScheduler> selectTheBestScheduler(const std::vector<std::shared_ptr<CustomScheduler>>& schedulers);
