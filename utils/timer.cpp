//
// Created by Morvan on 2022/7/3.
//

#include "timer.h"


namespace timer {
    [[noreturn]] void wakeEvery(int seconds, std::condition_variable& cv) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            cv.notify_all();
        }
    }
}