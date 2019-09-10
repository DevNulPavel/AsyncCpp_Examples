#pragma once

#include <atomic>

class SpinMutex{
public:
    SpinMutex();
    ~SpinMutex();
    SpinMutex(const SpinMutex&) = delete;
    SpinMutex& operator=(const SpinMutex&) = delete;
    
    void lock();
    void unlock();
    
private:
    std::atomic_flag _lock;
};

// https://github.com/yohhoy/yamc/blob/master/include/ttas_spin_mutex.hpp
class SpinMutexAdvanced {
public:
    SpinMutexAdvanced();
    ~SpinMutexAdvanced();
    SpinMutexAdvanced(const SpinMutexAdvanced&) = delete;
    SpinMutexAdvanced& operator=(const SpinMutexAdvanced&) = delete;
    
    void lock();
    bool try_lock();
    void unlock();
    
private:
    std::atomic<int> _state;
};
