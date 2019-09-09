#include <stdlib.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>

// Пример непосредственно с сайта https://github.com/Amanieu/asyncplusplus

void taskSpawnTest() {
    srand(time(NULL));
    
    // Создаем задачу 1
    auto task1 = async::spawn([] () {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::cout << "Task 1 executes asynchronously" << std::endl;
    });
    
    // Создаем задачу 2
    auto task2 = async::spawn([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Task 2 executes in parallel with task 1" << std::endl;
        return 42;
    });
    
    // Создаем задачу 3, которая выполнится после завершения второго таска
    // в качестве параметра передается результат из 2й задачи,
    // либо может быть передан непосредственно таск
    auto task3 = task2.then([](int value) -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Task 3 executes after task 2, which returned " << value << std::endl;
        return value * 3;
    });
    
    // Создаем задачу ожидания результата работы 1й и 2->3й задачи
    auto task4 = async::when_all(task1, task3);
    
    // Создаем продолжение ожидания задач, в качестве параметров - передается набор предыдущих тасков
    auto task5 = task4.then([](std::tuple<async::task<void>, async::task<int>> results) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Task 5 executes after tasks 1 and 3. Task 3 returned " << std::get<1>(results).get() << std::endl;
    });
    
    // Создаем задачу со случайной длительностью исполнения
    auto task6 = async::spawn([](){
        uint32_t randomDuration = (rand() & 800);
        std::this_thread::sleep_for(std::chrono::milliseconds(randomDuration));
        std::cout << "Task 6 executes with random delay " << randomDuration << "ms" << std::endl;
    });
    
    // Создаем задачу ожидания завершения хотя бы одной задачи
    auto task7 = async::when_any(task5, task6);
    
    // Непосредственное получение результата из задачи 7
    auto task8 = task7.then([](async::when_any_result<std::tuple<async::task<void>, async::task<void>>> results) {
        // Индекс задачи, которая была завершена первой
        std::size_t index = results.index;
        switch (index) {
            case 0:
                std::cout << "Task 5 has been executed first" << std::endl;
                break;
            case 1:
                std::cout << "Task 6 has been executed first" << std::endl;
                break;
        }
    });
    
    // Непосредственное получение результата из задачи 7 вместо продолжения
    //async::when_any_result<std::tuple<async::task<void>, async::task<void>>> results = task7.get();
       
    std::cout << "Exit from task spawn test" << std::endl;
}
