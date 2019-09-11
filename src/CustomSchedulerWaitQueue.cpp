#include "CustomSchedulerWaitQueue.h"

#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>

CustomSchedulerWaitQueue::CustomSchedulerWaitQueue():
    _stopWaiting(false){
}

CustomSchedulerWaitQueue::~CustomSchedulerWaitQueue(){
    std::cout << "Custom scheduler CustomSchedulerWaitQueue destructor exit" << std::endl;
}

void CustomSchedulerWaitQueue::waitAndPerformAllTasks(){
    // Ждем поступления новых задач
    std::unique_lock<std::mutex> lock(_mutex);
    _condVar.wait(lock, [this](){
        bool workExists = (_queue.size() > 0);
        bool needExit = _stopWaiting;
        return workExists || needExit;
    });
    
    // Mutex снова залочен
    
    // При завершении работы - обрабатываем очередь до конца
    while (_queue.size() > 0) {
        async::task_run_handle handle = std::move(_queue.front());
        _queue.pop();
        lock.unlock(); // Mutex разблокируем перед очередным выполнением
        
        handle.run();
        
        lock.lock(); // Заново блокируем, если будет итерация заново
    }
}

void CustomSchedulerWaitQueue::stopWaiting(){
    _stopWaiting = true;
    _condVar.notify_all();
}

bool CustomSchedulerWaitQueue::isStopRequested() const{
    return _stopWaiting;
}

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
void CustomSchedulerWaitQueue::schedule(async::task_run_handle handle) {
    if (_stopWaiting == true) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(std::move(handle));
    lock.unlock();
    _condVar.notify_all();
}
