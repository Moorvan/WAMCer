//
// Created by Morvan on 2022/7/3.
//

#ifndef WAMCER_TIMER_H
#define WAMCER_TIMER_H

#include <thread>
#include <chrono>
#include "utils/logger.h"

namespace timer {
    [[noreturn]] void wakeEvery(long long seconds, std::condition_variable& cv);
}


#endif //WAMCER_TIMER_H
