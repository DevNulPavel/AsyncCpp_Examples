#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>
#include "SpinMutex.h"


SpinMutexAdvanced mainThreadContextMutex;
auto mainThreadContextLastTask = async::make_task();
bool mainThreadExit = false;
int32_t mainContextValue = 0;

SpinMutexAdvanced fileContextMutex;
auto fileContextLastTask = async::make_task();
int32_t fileContextValue = 0;

SpinMutexAdvanced networkContextMutex;
auto networkContextLastTask = async::make_task();
int32_t networkContextValue = 0;

////////////////////////////////////////////////////////////////////////////////////////////////

void performInMainContext(std::function<void()>&& function){
    std::unique_lock<SpinMutexAdvanced> lock(mainThreadContextMutex);
    mainThreadContextLastTask = mainThreadContextLastTask.then(function);
}

void performInFileContext(std::function<void()>&& function){
    std::unique_lock<SpinMutexAdvanced> lock(fileContextMutex);
    fileContextLastTask = fileContextLastTask.then(function);
}

void performInNetworkContext(std::function<void()>&& function){
    std::unique_lock<SpinMutexAdvanced> lock(networkContextMutex);
    networkContextLastTask = networkContextLastTask.then(function);
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
        if(mainThreadContextLastTask.valid() && !mainThreadContextLastTask.ready()){
            mainThreadContextLastTask.wait();
        }
    }
    
    // Для окончательного завершения всех задач главного потока
    if(mainThreadContextLastTask.valid()){
        mainThreadContextLastTask.get();
    }
    
    std::cout << "Main thread exit: " << mainContextValue++ << std::endl;
}
