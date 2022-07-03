//
// Created by Morvan on 2022/7/3.
//

#ifndef WAMCER_TIMER_H
#define WAMCER_TIMER_H

#include <thread>
#include <chrono>

namespace timer {
    [[noreturn]] void wakeEvery(int seconds, std::condition_variable& cv);
}


#endif //WAMCER_TIMER_H
