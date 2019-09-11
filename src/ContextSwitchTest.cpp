#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <initializer_list>
#include <async++.h>
#include "SpinMutex.h"

class Context{
public:
    Context(){
        _lastTask = async::make_task();
    }
    async::task<void>& getTask(){
        return _lastTask;
    }
    
    void perform(std::function<void()>&& function){
        std::unique_lock<SpinMutexAdvanced> lock(_mutex);
        _lastTask = _lastTask.then(function);
    }
    void perform(const std::function<void()>& function){
        std::unique_lock<SpinMutexAdvanced> lock(_mutex);
        _lastTask = _lastTask.then(function);
    }
    void perform(const std::function<void()>& function, const std::function<void()>& completeFunction){
        std::unique_lock<SpinMutexAdvanced> lock(_mutex);
        _lastTask = _lastTask.then(function).then(completeFunction);
    }
    
private:
    SpinMutexAdvanced _mutex;
    async::task<void> _lastTask;
};

////////////////////////////////////////////////////////////////////////////////////////////////

Context mainThreadContext;
bool mainThreadExit = false;
int32_t mainContextValue = 0;

Context fileThreadContext;
int32_t fileContextValue = 0;

Context networkThreadContext;
int32_t networkContextValue = 0;

////////////////////////////////////////////////////////////////////////////////////////////////

void performInMainContext(std::function<void()>&& function){
    mainThreadContext.perform(std::move(function));
}

void performInFileContext(std::function<void()>&& function){
    fileThreadContext.perform(std::move(function));
}

void performInNetworkContext(std::function<void()>&& function){
    networkThreadContext.perform(std::move(function));
}

void runTasksOnContextsParallel(std::initializer_list<std::pair<Context&, std::function<void()>&>> tasksList){
    for (auto& task: tasksList) {
        task.first.perform(task.second);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

void testContextsSwitch() {
    
    performInFileContext([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Task in file context: " << fileContextValue++ << std::endl;
        
        performInNetworkContext([]{
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Task in network context: " << networkContextValue++ << std::endl;
            
            performInMainContext([](){
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "Task in main context: " << mainContextValue++ << std::endl;
                mainThreadExit = true;
            });
        });
    });
    
    performInFileContext([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Task in file context: " << fileContextValue++ << std::endl;
        
        performInMainContext([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Task in main context: " << mainContextValue++ << std::endl;
            
            performInNetworkContext([]{
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                std::cout << "Task in network context: " << networkContextValue++ << std::endl;
                
                performInMainContext([](){
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    std::cout << "Task in main context: " << mainContextValue++ << std::endl;
                });
            });
        });
    });
    
    performInFileContext([](){
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Task in file context: " << fileContextValue++ << std::endl;
        
        performInMainContext([](){
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "Task in main context: " << mainContextValue++ << std::endl;
        });
    });
    
    while (mainThreadExit == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if(mainThreadContext.getTask().valid() && !mainThreadContext.getTask().ready()){
            mainThreadContext.getTask().wait();
        }
    }
    
    // Для окончательного завершения всех задач главного потока
    if(mainThreadContext.getTask().valid()){
        mainThreadContext.getTask().get();
    }
    
    std::cout << "Main thread exit: " << mainContextValue++ << std::endl;
}
