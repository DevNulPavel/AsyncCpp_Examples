#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>
#include "Helpers.h"
#include "CustomSchedulerContextInPool.h"

///////////////////////////////////////////////////////////////////////////

void schedulersContextTest() {
    srand(time(NULL));
    
    // Variables
    std::shared_ptr<CustomSchedulerThreadPool> customSchedulerThreadPool;
    std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext1;
    std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext2;
    std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext3;
    int32_t threadContext1Var = 0;
    int32_t threadContext2Var = 0;
    int32_t threadContext3Var = 0;
    
    // Schedulers
    async::fifo_scheduler mainThreadScheduler;
    bool mainThreadExit = false;
    
    // Создаем кастомный пул + контексты очереди, которые будут работать на этому пуле
    customSchedulerThreadPool = std::make_shared<CustomSchedulerThreadPool>(8);
    customSchedulerThreadContext1 = customSchedulerThreadPool->makeNewContext();
    customSchedulerThreadContext2 = customSchedulerThreadPool->makeNewContext();
    customSchedulerThreadContext3 = customSchedulerThreadPool->makeNewContext();
    
    // Специальный таск-ивент, для передачи результата
    async::event_task<int32_t> eventObject;
    auto task0 = eventObject.get_task();
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task1 = task0.then(*customSchedulerThreadContext1.lock(), [&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 1 executes asynchronously in custom scheduler context 1" << std::endl);
        threadContext1Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2 = task1.then(*customSchedulerThreadContext2.lock(), [&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task3 = task2.then(*customSchedulerThreadContext3.lock(), [&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 3 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext3Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task4 = task3.then(*customSchedulerThreadContext2.lock(), [&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 4 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    // Дожидаемся результата тех самых 2х задач
    //auto task4 = async::when_all(task1, task2, task3); // Thread sanitizer ругается на when_all
    
    // Выход из главного потока в конце
    task4.then(mainThreadScheduler, [&](){
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
    customSchedulerThreadPool->stopAll();
    customSchedulerThreadPool = nullptr;
    
    LOG(std::cout << "Main thread exit" << std::endl);
}

///////////////////////////////////////////////////////////////////////////

void schedulersContextPerformanceTest() {
    srand(time(NULL));

    struct ContextEmulation{
        std::weak_ptr<CustomSchedulerContextInPool> context;
        int64_t variable1;
        int64_t variable2;
        
        ContextEmulation(std::weak_ptr<CustomSchedulerContextInPool> srcContext):
            context(srcContext),
            variable1(0),
            variable2(0){
        }
    };
    
    // Variables
    std::shared_ptr<CustomSchedulerThreadPool> customSchedulerThreadPool = std::make_shared<CustomSchedulerThreadPool>(8);
    std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContexts;
    
    // Создаем кастомный пул + контексты очереди, которые будут работать на этому пуле
    std::vector<ContextEmulation> contexts;
    
    const size_t totalCount = 500000;
    contexts.reserve(totalCount);
    for (size_t i = 0; i < totalCount; i++) {
        auto context = customSchedulerThreadPool->makeNewContext();
        ContextEmulation contextEmu(context);
        contexts.push_back(std::move(contextEmu));
    }
    
    LOG(std::cout << "Contexts created" << std::endl);
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    LOG(std::cout << "Sleep completed" << std::endl);
    
    // Базовая задача, которая стартует остальные
    async::event_task<bool> mainEvent;
    auto mainSharedTask = mainEvent.get_task().share();
    
    for (size_t i = 0; i < totalCount; i += 4) {
        // Продолжаем задачу в контексте файлового шедулера
        ContextEmulation& emu1 = contexts[i];
        auto task1 = mainSharedTask.then(*emu1.context.lock(), [&](){
            emu1.variable1 += 1;
            emu1.variable2 += 1;
        });
        
        // Продолжаем задачу в контексте файлового шедулера
        ContextEmulation& emu2 = contexts[i+1];
        auto task2 = task1.then(*emu2.context.lock(), [&](){
            emu2.variable1 += 1;
            emu2.variable2 += 1;
        });
        
        // Продолжаем задачу в контексте файлового шедулера
        ContextEmulation& emu3 = contexts[i+2];
        auto task3 = task2.then(*emu3.context.lock(), [&](){
            //customSchedulerThreadPool->removeContext(emu3.context);
            //emu3.context = customSchedulerThreadPool->makeNewContext();
            
            emu3.variable1 += 1;
            emu3.variable2 += 1;
        });
        
        // Продолжаем задачу в контексте файлового шедулера
        ContextEmulation& emu4 = contexts[i+3];
        auto task4 = task3.then(*emu4.context.lock(), [&](){
            emu4.variable1 += 1;
            emu4.variable2 += 1;
        });
    }
    
    LOG(std::cout << "Execution time test" << std::endl);
    mainEvent.set(true);
    
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    LOG(std::cout << "Schedulers delete" << std::endl);
    customSchedulerThreadPool->stopAll();
    customSchedulerThreadPool = nullptr;
    
    
    // Thread sanitizer ругается
    /*int64_t totalPerformed = async::parallel_reduce(async::irange((size_t)0, totalCount), 0, [&](size_t i, size_t j){
        const ContextEmulation& emu1 = contexts[i];
        const ContextEmulation& emu2 = contexts[j];
        return emu1.variable1 + emu2.variable2;
    });*/
    /*int64_t totalPerformed = 0;
    for (size_t i = 0; i < totalCount; i++) {
        totalPerformed += contexts[i].variable1;
    }*/
    std::vector<async::task<int64_t>> futures;
    futures.reserve(std::thread::hardware_concurrency());
    const size_t step = totalCount / std::thread::hardware_concurrency();
    for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
        size_t begin = i*step;
        size_t end = i*step + step;
        if (i == std::thread::hardware_concurrency()-1) {
            end += totalCount % std::thread::hardware_concurrency();
        }
        auto future = async::spawn([&, begin, end](){
            int64_t totalPerformed = 0;
            for (size_t i = begin; i < end; i++) {
                totalPerformed += contexts[i].variable1;
            }
            return totalPerformed;
        });
        futures.push_back(std::move(future));
    }
    int64_t totalPerformed = 0;
    for (auto& future: futures) {
        totalPerformed += future.get();
    }
    LOG(std::cout << "Total performed: " << totalPerformed << std::endl);
    
    LOG(std::cout << "Main thread exit" << std::endl);
}
