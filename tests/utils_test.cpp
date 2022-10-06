//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include <future>
#include "utils/timer.h"
#include "utils/logger.h"
#include "config.h"

using namespace wamcer;

TEST(TimerTest, WakeEvery) {
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto cv2 = std::condition_variable();
    auto t = std::thread([&] {
        auto lck = std::unique_lock(mux);
        logger.log(0, "lck in t");
        cv.wait(lck);
        logger.log(0, "unlock!!");
        while (true) {
            logger.log(0, "Print");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cv2.notify_all();
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
    auto mux2 = std::mutex();
    auto lck = std::unique_lock(mux);
    logger.log(0, "lock in main.");
    cv2.wait(lck);
    logger.log(0, "unlock in main!!");
    t.join();
    t1.join();
}

TEST(DetachTester, Detach) {
    auto i = 1;
    auto signal_exit = std::promise<void>();
    auto exit_future = signal_exit.get_future();
    auto t = std::thread([&] {
        logger.log(0, "This is in t");
        while (exit_future.wait_for(std::chrono::microseconds(1)) == std::future_status::timeout) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            logger.log(0, "t: i = {}", i);
            i++;
        }
        logger.log(0, "t: exit");
    });
    std::this_thread::sleep_for(std::chrono::seconds(10));
    signal_exit.set_value();
    t.detach();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        logger.log(0, "main: i = {}", i);
    }
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