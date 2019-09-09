#include <iostream>
#include <thread>
#include <chrono>
#include <async++.h>

// Пример непосредственно с сайта https://github.com/Amanieu/asyncplusplus

void parallelAlgoTest() {
    // Просто параллельный запуск задачи
    async::parallel_invoke([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::cout << "This is executed in parallel..." << std::endl;
    }, [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "with this" << std::endl;
    });
    
    // Запуск цикла for для диапазона значений
    async::parallel_for(async::irange(0, 5), [](int x) {
        std::cout << x;
    });
    std::cout << std::endl;
    
    // Запуск задачи сверки в параллель
    int r = async::parallel_reduce({1, 2, 3, 4}, 0, [](int x, int y) {
        return x + y;
    });
    std::cout << "The sum of {1, 2, 3, 4} is " << r << std::endl;
}
