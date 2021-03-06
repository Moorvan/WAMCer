//
// Created by Morvan on 2022/7/3.
//

#include "timer.h"


namespace timer {
    [[noreturn]] void wakeEvery(long long seconds, std::condition_variable& cv) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(seconds));
            cv.notify_all();
        }
    }
}