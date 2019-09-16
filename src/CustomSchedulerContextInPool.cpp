#include "CustomSchedulerContextInPool.h"
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>
#include "Helpers.h"


CustomSchedulerThreadPool::CustomSchedulerThreadPool(size_t threadsCount):
    _needStop(false),
    _threadsCompleted(0){
    
    _threads.reserve(threadsCount);
    
    auto threadFunction = [this](){
        while (_needStop == false) {
            // Блокировка
            std::unique_lock<std::mutex> lock(_mutex);
            
            // Ждем поступления новой работы или запроса завершения, на время ожидания Mutex разлочен
            _condVar.wait(lock, [this](){
                // Тут Mutex снова заблокирован
                bool workExists = (_contextsExecutionQueue.size() > 0);
                bool needStop = _needStop;
                return workExists || needStop;
            });
            
            // Mutex снова залочен
            
            // Проверяем, не выход ли это?
            if (_needStop == false) {
                // Выполняем таски до тех пор, пока у нас во всех контекстах не закончатся задачи
                while(_contextsExecutionQueue.size() > 0){
                    // Первично извлекаем контекст для исполнения
                    std::weak_ptr<CustomSchedulerContextInPool> context = std::move(_contextsExecutionQueue.front());
                    _contextsExecutionQueue.pop_front();
                    
                    // Проверяем, что он не истек
                    if (context.expired() == false) {
                        // Получаем фактический контекст
                        std::shared_ptr<CustomSchedulerContextInPool> contextShared = context.lock();
                        
                        // На время работы тасков - разблокируем
                        lock.unlock();
                        
                        // Была ли выполнена какая-то задача, либо задачи закончились, или мы уже на исполнении этой очереди
                        bool executed = contextShared->performOneTask();
                        
                        // Для новой итерации снова ставим блокировку
                        lock.lock();
                        
                        // Если задача была выполнена, то ставим в очередь еще одну проверку очереди,
                        // Если задача была на исполнении уже в другом потоке, то он и отвечает за повторную проверку - это нужно,
                        // чтобы не терялись задачи в случае, если несколько потоков начинают обрабатывать одну очередь
                        // Немного избыточный подход, зато надежный
                        if (executed) {
                            _contextsExecutionQueue.push_back(context);
                        }else{
                            //LOG(std::cout << "Task context in execution in other thread or context empty" << std::endl);
                        }
                    }
                }
            }
            
            // Mutex снова залочен
            
            //LOG(std::cout << "Start thread waiting" << std::endl);
        }
        
        LOG(std::cout << "Custom scheduler context thread exit" << std::endl);
        _threadsCompleted++;
        _exitCondVar.notify_all();
    };
    
    for (size_t i = 0; i < threadsCount; i++) {
        std::thread workThread(threadFunction);
        workThread.detach();
        _threads.push_back(std::move(workThread));
    }
}

CustomSchedulerThreadPool::~CustomSchedulerThreadPool(){
    if (_needStop == true) {
        return;
    }
    
    _needStop = true;
    _condVar.notify_all();
    
    size_t threadsCount = _threads.size();
    std::unique_lock<std::mutex> lock(_mutex);
    _exitCondVar.wait(lock, [this, threadsCount](){
        // Тут Mutex снова заблокирован
        return _threadsCompleted == threadsCount;
    });
    
    LOG(std::cout << "Custom scheduler thread pool destructor exit" << std::endl);
}

std::weak_ptr<CustomSchedulerContextInPool> CustomSchedulerThreadPool::makeNewContext(){
    if (_needStop) {
        return std::weak_ptr<CustomSchedulerContextInPool>();
    }
    
    CustomSchedulerContextInPool* rawPtr = new CustomSchedulerContextInPool(shared_from_this());
    std::shared_ptr<CustomSchedulerContextInPool> context(rawPtr);
    
    std::unique_lock<std::mutex> lock(_mutex);
    _contexts.push_back(context);
    lock.unlock();
    
    wakeUp();
    
    return std::weak_ptr<CustomSchedulerContextInPool>(context);
}

void CustomSchedulerThreadPool::removeContext(const std::weak_ptr<CustomSchedulerContextInPool>& context){
    std::unique_lock<std::mutex> lock(_mutex);
    if (context.expired() == false) {
        std::shared_ptr<CustomSchedulerContextInPool> contextPtr = context.lock();
        _contexts.remove(contextPtr);
    }
    lock.unlock();
    
    wakeUp();
}

void CustomSchedulerThreadPool::wakeUp(){
    _condVar.notify_one();
}

void CustomSchedulerThreadPool::wakeUpForContext(const std::weak_ptr<CustomSchedulerContextInPool>& context){
    std::unique_lock<std::mutex> lock(_mutex);
    _contextsExecutionQueue.push_back(context);
    lock.unlock();
    
    _condVar.notify_one();
}

///////////////////////////////////////////////////////////////////////////////////////////

CustomSchedulerContextInPool::CustomSchedulerContextInPool(const std::weak_ptr<CustomSchedulerThreadPool>& pool):
    _threadPool(pool),
    _executionInProgress(false){
}

CustomSchedulerContextInPool::~CustomSchedulerContextInPool(){
}

// Для написания кастомного шедулера достаточно, чтобы класс реализовывал метод schedule
void CustomSchedulerContextInPool::schedule(async::task_run_handle handle) {
    std::unique_lock<SpinMutex> lock(_taskQueueMutex);
    _taskQueue.push(std::move(handle));
    lock.unlock();
    
    if (_threadPool.expired() == false) {
        _threadPool.lock()->wakeUpForContext(shared_from_this());
    }
}

// На время исполнения задачи - блокируемся
bool CustomSchedulerContextInPool::performOneTask() {
    // Пробуем заблокироваться, если не вышло - значит работа уже идет где-то в другом потоке
    bool lockSuccess = _executionQueueMutex.try_lock();
    if (lockSuccess == false) {
        return false;
    }
    
    // Создаем блокировку с adopt_lock, так как у нас уже есть блокировка
    std::unique_lock<std::mutex> executionLock(_executionQueueMutex, std::adopt_lock);
    
    // Быстренько перекидываем задачи из входящей очереди в очередь исполнения и освобождаем, чтобы можно было добавлять задачи даже во время исполнениия
    std::unique_lock<SpinMutex> queueLock(_taskQueueMutex);
    while (_taskQueue.size() > 0) {
        async::task_run_handle handle = std::move(_taskQueue.front());
        _taskQueue.pop();
        _executionQueue.push(std::move(handle));
    }
    queueLock.unlock();

    // Выполняем одну задачу из очереди исполнения, при этом у нас висит блокировка исполнения, задачи будут исполняться гарантированно последовательно
    if(_executionQueue.size() > 0){
        async::task_run_handle handle = std::move(_executionQueue.front());
        _executionQueue.pop();
        if (handle) {
            handle.run();
        }
        return true;
    }
    
    return false;
}

    
