#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>
#include "Helpers.h"
#include "CustomSchedulerContextInPool.h"


static std::shared_ptr<CustomSchedulerThreadPool> customSchedulerThreadPool;
static std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext1;
static std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext2;
static std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext3;
static int32_t threadContext1Var = 0;
static int32_t threadContext2Var = 0;
static int32_t threadContext3Var = 0;

static async::fifo_scheduler mainThreadScheduler;
static bool mainThreadExit = false;

///////////////////////////////////////////////////////////////////////////

void schedulersContextTest() {
    srand(time(NULL));
    
    // Создаем кастомный пул + контексты очереди, которые будут работать на этому пуле
    customSchedulerThreadPool = std::make_shared<CustomSchedulerThreadPool>(8);
    customSchedulerThreadContext1 = customSchedulerThreadPool->makeNewContext();
    customSchedulerThreadContext2 = customSchedulerThreadPool->makeNewContext();
    customSchedulerThreadContext3 = customSchedulerThreadPool->makeNewContext();
    
    // Специальный таск-ивент, для передачи результата
    async::event_task<int32_t> eventObject;
    auto task0 = eventObject.get_task();
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task1 = task0.then(*customSchedulerThreadContext1.lock(), [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 1 executes asynchronously in custom scheduler context 1" << std::endl);
        threadContext1Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2 = task1.then(*customSchedulerThreadContext2.lock(), [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task3 = task2.then(*customSchedulerThreadContext3.lock(), [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 3 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext3Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task4 = task3.then(*customSchedulerThreadContext2.lock(), [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 4 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    // Дожидаемся результата тех самых 2х задач
    //auto task4 = async::when_all(task1, task2, task3); // Thread sanitizer ругается на when_all
    
    // Выход из главного потока в конце
    task4.then(mainThreadScheduler, [](){
        mainThreadExit = true;
    });
    
    // Так мы апускаем задачи
    eventObject.set(1);
    
    while (mainThreadExit == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Данный обработчик не блокируется на ожидании задач, если нет - выходит
        // аналогично работает и run_one_task();
        mainThreadScheduler.run_all_tasks();
    }
    
    LOG(std::cout << "Schedulers delete" << std::endl);
    customSchedulerThreadPool = nullptr;
    
    LOG(std::cout << "Main thread exit" << std::endl);
}
