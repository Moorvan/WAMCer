//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include "utils/timer.h"
#include "utils/logger.h"
#include "config.h"
using namespace wamcer;

TEST(TimerTest, WakeEvery) {
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto t = std::thread([&] {
        auto lck = std::unique_lock(mux);
        logger.log(0, "lck in t");
        cv.wait(lck);
        while (true) {
            logger.log(0, "Print");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    auto t1 = std::thread([&] {
        logger.log(0, "This is in t1");
        while (true) {
            logger.log(0, "wait");
            logger.log(0, "locked?");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cv.notify_all();
        }
    });

    t.join();
    t1.join();
}

TEST(ConfigTest, GetValue) {
    logger.log(0, "wakeKindCycle: {}", config::wakeKindCycle);

}

TEST(LoggerTest, Verbose) {
    logger.set_verbosity(1);
    auto t0 = std::thread([&] {
        logger.set_verbosity(2);
    });
    t0.join();
    logger.log(2, "adaffsdadf");
}