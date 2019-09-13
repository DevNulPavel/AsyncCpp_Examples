#pragma once

#include <mutex>

static std::mutex ioLock;
#define LOG(CODE) {std::unique_lock<std::mutex> lk(ioLock); CODE; }
