#include "CustomSchedulerBalancer.h"

#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>

CustomSchedulerBalancer::CustomSchedulerBalancer():
    _stopThread(false),
    _taskIsInWork(false),
    _threadExitSuccess(false){
    
    std::thread workThread([this](){
        while (_stopThread == false) {
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [this](){
                bool workExists = (_queue.size() > 0);
                bool needExit = _stopThread;
                return workExists || needExit;
            });
            
            // Mutex снова залочен
            
            // Есть ли работа, или у нас просто выход
            if (_queue.size() > 0) {
                // При завершении работы - обрабатываем очередь до конца
                do {
                    async::task_run_handle handle = std::move(_queue.front());
                    _queue.pop();
                    lock.unlock(); // Mutex разблокируем перед очередным выполнением
                    
                    _taskIsInWork = true;
                    handle.run();
                    _taskIsInWork = false;
                    
                    if (_stopThread == true) {
                        lock.lock(); // Заново блокируем, если будет итерация заново
                    }
                } while ((_stopThread == true) && (_queue.size() > 0));
            }
        }
        
        std::cout << "Custom scheduler thread exit" << std::endl;
        _threadExitSuccess = true;
        _condVarExit.notify_all();
    });
    workThread.detach();
}

CustomSchedulerBalancer::~CustomSchedulerBalancer(){
    _stopThread = true;
    _condVar.notify_all();
    
    // Дождемся завершения шедулера
    std::unique_lock<std::mutex> lock(_mutex);
    _condVarExit.wait(lock, [this](){
        return (_threadExitSuccess == true);
    });
    std::cout << "Custom scheduler destructor exit" << std::endl;
}

size_t CustomSchedulerBalancer::getQueueSize(){
    std::unique_lock<std::mutex> lock(_mutex);
    return _queue.size();
}

bool CustomSchedulerBalancer::taskIsInWork() const{
    return _taskIsInWork;
}

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
void CustomSchedulerBalancer::schedule(async::task_run_handle handle) {
    if (_stopThread == true) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(_mutex);
    _queue.push(std::move(handle));
    lock.unlock();
    _condVar.notify_all();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<CustomSchedulerBalancer> selectTheBestScheduler(const std::vector<std::shared_ptr<CustomSchedulerBalancer>>& schedulers){
    if (schedulers.size() == 0) {
        return nullptr;
    }
    
    // Так как данные могут меняться, то заранее подготавливаем инфу для сортировки
    struct SortStruct{
        std::shared_ptr<CustomSchedulerBalancer> shed;
        size_t size;
        bool inWork;
    };
    
    std::vector<SortStruct> schedulersInfo;
    schedulersInfo.reserve(schedulers.size());
    for (const auto& sched: schedulers) {
        SortStruct sortInfo{
            sched,
            sched->getQueueSize(),
            sched->taskIsInWork()
        };
        schedulersInfo.push_back(std::move(sortInfo));
    }
    
    std::function<bool(const SortStruct& s1, const SortStruct& s2)> sortFunc([](const SortStruct& s1, const SortStruct& s2){
        if (s1.size < s2.size) {
            return true;
        }
        if ((s1.inWork == false) && (s1.inWork == true)) {
            return true;
        }
        // Чтобы сортировка не падала при одинаковых значениях свойств и все было однозначно, то просто сравниваем адреса
        if (&s1 < &s2) {
            return true;
        }
        return false;
    });
    std::sort(schedulersInfo.begin(), schedulersInfo.end(), sortFunc);
    return schedulersInfo.front().shed;
}
