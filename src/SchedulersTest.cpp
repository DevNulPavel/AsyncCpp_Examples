#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>
#include "Helpers.h"
#include "CustomSchedulerBalancer.h"
#include "CustomSchedulerWaitQueue.h"
#include "CustomSchedulerContextInPool.h"


static std::shared_ptr<async::threadpool_scheduler> networkContextScheduler;
static int32_t networkTestCounter = 0;

static std::shared_ptr<async::threadpool_scheduler> fileContextScheduler;
static int32_t fileTestCounter = 0;

static std::shared_ptr<CustomSchedulerWaitQueue> customSchedulerWaitQueue;

static std::vector<std::shared_ptr<CustomSchedulerBalancer>> testContextCustomSchedulers;

static std::shared_ptr<CustomSchedulerThreadPool> customSchedulerThreadPool;
static std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext1;
static std::weak_ptr<CustomSchedulerContextInPool> customSchedulerThreadContext2;
static int32_t threadContext1Var = 0;
static int32_t threadContext2Var = 0;

static async::fifo_scheduler mainThreadScheduler;
static bool mainThreadExit = false;
static int32_t mainThreadCounter = 0;

///////////////////////////////////////////////////////////////////////////

void networkThreadFunctionBefore(){
    // Тут надо инициализировать как-то нужный нам контекст исполнения
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    LOG(std::cout << "Network thread: before" << std::endl);
}

void networkThreadFunctionAfter(){
    // Тут надо инициализировать как-то нужный нам контекст исполнения
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    LOG(std::cout << "Network thread: after" << std::endl);
}

///////////////////////////////////////////////////////////////////////////

void fileThreadFunctionBefore(){
    // Тут надо инициализировать как-то нужный нам контекст исполнения
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    LOG(std::cout << "File thread: before" << std::endl);
}

void fileThreadFunctionAfter(){
    // Тут надо инициализировать как-то нужный нам контекст исполнения
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    LOG(std::cout << "File thread: after" << std::endl);
}

///////////////////////////////////////////////////////////////////////////

void customSchedulerWaitQueueThreadFunction(){
    // Проверяем, нету ли задач
    while (customSchedulerWaitQueue->isStopRequested() == false) {
        customSchedulerWaitQueue->waitAndPerformAllTasks();
    }
    
    // Уничтожать должен именно данный поток
    customSchedulerWaitQueue = nullptr;
    LOG(std::cout << "Exit from thread customSchedulerWaitQueue" << std::endl);
}

///////////////////////////////////////////////////////////////////////////

void schedulersTest() {
    srand(time(NULL));
    
    // Создаем по одному потоку, чтобы иметь доступ к переменным контекста потока только из одного потока
    networkContextScheduler = std::make_shared<async::threadpool_scheduler>(1, networkThreadFunctionBefore, networkThreadFunctionAfter);
    fileContextScheduler = std::make_shared<async::threadpool_scheduler>(1, fileThreadFunctionBefore, fileThreadFunctionAfter);
    
    // Создаем наши кастомные шедулеры с поддержкой балансировки нагрузки
    const uint32_t schedulersCount = 4;
    testContextCustomSchedulers.reserve(schedulersCount);
    for (size_t i = 0; i < schedulersCount; i++) {
        testContextCustomSchedulers.push_back(std::make_shared<CustomSchedulerBalancer>());
    }
    
    // Создаем FIFI шедулер с ожиданием, владеть этим шедулером будет поток кастомного шедулера ниже
    customSchedulerWaitQueue = std::make_shared<CustomSchedulerWaitQueue>();
    
    // Создаем поток для кастомного шедулера
    std::thread customSchedulerThread(customSchedulerWaitQueueThreadFunction);
    customSchedulerThread.detach();
    
    // Создаем кастомный пул + контексты очереди, которые будут работать на этому пуле
    customSchedulerThreadPool = std::make_shared<CustomSchedulerThreadPool>(8);
    customSchedulerThreadContext1 = customSchedulerThreadPool->makeNewContext();
    customSchedulerThreadContext2 = customSchedulerThreadPool->makeNewContext();
    
    // Специальный ивент, для передачи результата
    async::event_task<int32_t> eventTask;
    
    // Создаем задачу на контексте сетевого шедулера
    auto task1_1 = async::spawn(*networkContextScheduler, [](){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 1_1 executes asynchronously in network thread" << std::endl);
        
        networkTestCounter++;
    });
    
    // Создаем задачу из евента
    auto task1_2 = eventTask.get_task();
    
    // Еще один кастомный шедулер
    auto task1_3 = async::spawn(*customSchedulerWaitQueue, []{
        LOG(std::cout << "Task 1_3 executes asynchronously in customSchedulerWaitQueue thread" << std::endl);
    });
    
    // Создаем задачу ожидания
    auto task1_wait = async::when_all(task1_1, task1_2, task1_3); // TODO: Thread санитайзер выдает гонку данных
    
    // В качестве продолжения мы можем назначить несколкьо задач одной задаче, для этого нужно вызывать share
    auto task1_shared = task1_wait.then([](std::tuple<async::task<void>, async::task<int>, async::task<void>> results){
    }).share();
    
    eventTask.set(1234);
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2_1 = task1_shared.then(*fileContextScheduler, [](async::shared_task<void> task1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2_1 executes asynchronously in file thread" << std::endl);
        
        fileTestCounter++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2_2 = task1_shared.then(*customSchedulerThreadContext1.lock(), [](async::shared_task<void> task1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2_2 executes asynchronously in custom scheduler context 1" << std::endl);
        threadContext1Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2_3 = task1_shared.then(*customSchedulerThreadContext2.lock(), [](async::shared_task<void> task1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2_3 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    // Продолжаем задачу в контексте файлового шедулера
    auto task2_4 = task1_shared.then(*customSchedulerThreadContext2.lock(), [](async::shared_task<void> task1){
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 2_4 executes asynchronously in custom scheduler context 2" << std::endl);
        
        threadContext2Var++;
    });
    
    
    // Дожидаемся результата тех самых 2х задач
    auto task2_wait = async::when_all(task2_1, task2_2, task2_3, task2_4); // TODO: Thread санитайзер выдает гонку данных
    
    // Простая балансировка шедулеров, выбор наиболее свободного на данный момент шедулера
    std::shared_ptr<CustomSchedulerBalancer> bestSched = selectTheBestScheduler(testContextCustomSchedulers);
    
    // Продолжаем работу в том самом лучшем шедулере
    auto task3 = task2_wait.then(*bestSched, [bestSched](){
        std::shared_ptr<int64_t> testVariablePtr = std::make_shared<int64_t>(0);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(std::cout << "Task 3 executes asynchronously in custom scheduler, variable: " << (*testVariablePtr) << ", thread: " << reinterpret_cast<size_t>(bestSched.get()) << std::endl);
        
        // После какой-то работы в кастомном шедулере, создаем новые какие-то задачи в контексте файлового шедулера
        auto internalTask1 = async::spawn(*fileContextScheduler, [testVariablePtr](){
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            LOG(std::cout << "Internal task 1 executes asynchronously in file thread" << std::endl);
            
            // Мы можем сделать подзадачу, результат которой будет развернут в качестве параметра следующей задачи от базовой
            return async::spawn(*networkContextScheduler, [] {
                LOG(std::cout << "Internal sub task 1 executes asynchronously in network thread" << std::endl);
                return 42;
            });
        });
        
        // Затем продолжаем работу в контексте ТОГО же самого контекста, имея доступ к переменным
        auto internalTask2 = internalTask1.then(*bestSched, [bestSched, testVariablePtr](int32_t prevResult){
            LOG(std::cout << "Internal task 2 executes asynchronously in custom scheduler, previous task variable: " << prevResult << ", thread: " << reinterpret_cast<size_t>(bestSched.get()) << std::endl);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (*testVariablePtr)++;
            LOG(std::cout << "Internal task 2 executes asynchronously in custom scheduler, variable: " << (*testVariablePtr) << ", thread: " << reinterpret_cast<size_t>(bestSched.get()) << std::endl);
        });
        
        // После какой-то работы в кастомном шедулере, создаем новые какие-то задачи в контексте файлового шедулера
        auto resultTaskMain = internalTask2.then(mainThreadScheduler, [](){
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            LOG(std::cout << "Result task executes in main thread" << std::endl);
            
            mainThreadCounter++;
            mainThreadExit = true;
        });
    });
    
    while (mainThreadExit == false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Данный обработчик не блокируется на ожидании задач, если нет - выходит
        // аналогично работает и run_one_task();
        mainThreadScheduler.run_all_tasks();
    }
    
    LOG(std::cout << "Schedulers delete" << std::endl);
    networkContextScheduler = nullptr;
    fileContextScheduler = nullptr;
    customSchedulerWaitQueue->stopWaiting(); // Данным шедулером управляет сторонний поток, тут можно только попросить его остановиться
    testContextCustomSchedulers.clear();
    customSchedulerThreadPool = nullptr;
    
    LOG(std::cout << "Main thread exit" << std::endl);
}
