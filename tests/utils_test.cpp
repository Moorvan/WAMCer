//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include "utils/timer.h"
#include "utils/logger.h"
using namespace wamcer;

TEST(TimerTest, WakeEvery) {
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto t = std::thread([&] {
        timer::wakeEvery(2, cv);
    });
    auto t1 = std::thread([&] {
        logger.log(0, "This is in t1");
        while (true) {
            logger.log(0, "t1");
            auto lck = std::unique_lock<std::mutex>(mux);
            cv.wait(lck);
        }
    });
    t.join();
    t1.join();
}