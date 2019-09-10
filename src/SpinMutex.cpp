#include "SpinMutex.h"

SpinMutex::SpinMutex():
    _lock(false){
}

SpinMutex::~SpinMutex(){
    unlock();
}
    
void SpinMutex::lock(){
    while (_lock.test_and_set(std::memory_order_acquire));
}

void SpinMutex::unlock() {
    _lock.clear(std::memory_order_release);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SpinMutexAdvanced::SpinMutexAdvanced():
    _state(0){
}

SpinMutexAdvanced::~SpinMutexAdvanced() {
    assert(_state.load() == 0);
}

void SpinMutexAdvanced::lock(){
    int expected;
    do {
        while (_state.load(std::memory_order_relaxed) != 0) {}
        expected = 0;
    } while (!_state.compare_exchange_weak(expected, 1, std::memory_order_acquire));
}

bool SpinMutexAdvanced::try_lock(){
    int expected = 0;
    return _state.compare_exchange_weak(expected, 1, std::memory_order_acquire);
}

void SpinMutexAdvanced::unlock() {
    _state.store(0, std::memory_order_release);
}

