//
// Created by Morvan on 2022/7/3.
//

#include "timer.h"


namespace timer {
    void wakeEvery(long long seconds, std::condition_variable &cv, bool &isFinished) {
        while (!isFinished) {
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
            cv.notify_all();
        }
        return;
    }
}