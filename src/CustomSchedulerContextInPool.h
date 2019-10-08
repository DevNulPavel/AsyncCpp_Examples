#pragma once

#include <atomic>
#include <thread>
#include <list>
#include <vector>
#include <queue>
#include <async++.h>
#include "SpinMutex.h"

/*
- Создавать/уничтожать пул потоков + создавать/уничтожать контексты нужно в одном и том же потоке
*/


class CustomSchedulerThreadPool;
class CustomSchedulerContextInPool;


class CustomSchedulerThreadPool: public std::enable_shared_from_this<CustomSchedulerThreadPool>{
    friend class CustomSchedulerContextInPool;
    
public:
    CustomSchedulerThreadPool(size_t threadsCount);
    ~CustomSchedulerThreadPool();
    void stopAll();
    std::weak_ptr<CustomSchedulerContextInPool> makeNewContext();
    void removeContext(const std::weak_ptr<CustomSchedulerContextInPool>& context);
    
private:
    std::vector<std::thread> _threads;
    std::mutex _mutex;
    std::condition_variable _condVar;
    std::list<std::shared_ptr<CustomSchedulerContextInPool>> _contexts;
    std::queue<std::weak_ptr<CustomSchedulerContextInPool>> _contextsExecutionQueue;
    std::atomic_bool _needStop;
    std::mutex _exitMutex;
    std::condition_variable _exitCondVar;
    size_t _threadsCompleted;

private:
    void wakeUp();
    void wakeUpForContext(const std::weak_ptr<CustomSchedulerContextInPool>& context);
};

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
class CustomSchedulerContextInPool: public std::enable_shared_from_this<CustomSchedulerContextInPool> {
    friend class CustomSchedulerThreadPool;
    
public:
    void schedule(async::task_run_handle t);
    virtual ~CustomSchedulerContextInPool();
    void clearAllTasks();
    
private:
    std::weak_ptr<CustomSchedulerThreadPool> _threadPool;
    std::mutex _taskQueueMutex; // Можно использовать SpinMutex
    std::queue<async::task_run_handle> _taskQueue;
    std::atomic_bool _executionInProgress;
    
private:
    // Конструктор специально приватный, так как создавать контекст может только пул
    CustomSchedulerContextInPool(const std::weak_ptr<CustomSchedulerThreadPool>& pool);
    bool performOneTask();
};
