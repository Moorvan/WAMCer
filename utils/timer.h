//
// Created by Morvan on 2022/7/3.
//

#ifndef WAMCER_TIMER_H
#define WAMCER_TIMER_H

#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include "utils/logger.h"

namespace timer {
    void wakeEvery(long long seconds, std::condition_variable& cv, bool& isFinished);
}


#endif //WAMCER_TIMER_H
