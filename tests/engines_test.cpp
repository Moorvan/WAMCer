//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include <core/runner.h>
#include <engines/k_induction.h>
#include <thread>
#include <chrono>
#include "utils/timer.h"
#include "config.h"

using namespace wamcer;

TEST(BMCTests, SafeStep) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    logger.log(0, "file: {}.", path);
    logger.log(0, "BMC running...");
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMC(ts, p);
    auto t = std::thread([&] {
        bmc.run();
    });
    t.join();
}

TEST(BMCTests, Sync) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    logger.log(0, "file: {}.", path);
    logger.log(0, "BMC running...");
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto step = int();
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMC(ts, p, step, mux, cv);
    auto t = std::thread([&] {
        bmc.run(-1);
    });
    using namespace std::chrono_literals;
    logger.log(0, "sleep for 2s");
    std::this_thread::sleep_for(2s);
    logger.log(0, "notify_all()");
    cv.notify_all();
    t.join();
}

TEST(KInductionTests, SingleKInd) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-101.btor2";
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto tsCopy = TransitionSystem::copy(ts);
    auto pCopy = tsCopy.add_term(p);
    auto kind = KInduction(tsCopy, pCopy);
    if (kind.run()) {
        logger.log(0, "Proved: IS INVARIANT.");
    } else {
        logger.log(0, "Unknown.");
    }
}

TEST(KInductionWithBMC, KindWithBMC) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-true.btor";
    auto ts1 = TransitionSystem();
    auto ts2 = TransitionSystem();
    auto p1 = BTOR2Encoder(path, ts1).propvec().at(0);
    auto p2 = BTOR2Encoder(path, ts2).propvec().at(0);
    auto safe = int();
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto isFinished = std::condition_variable();
    auto kind = KInduction(ts1, p1, safe, mux, cv);
    auto bmc = BMC(ts2, p2, safe, mux, cv);
    auto wake = std::thread([&] {
        timer::wakeEvery(config::wakeKindCycle, cv);
    });
    auto bmcRes = bool();
    auto kindRes = bool();
    auto finishedID = std::atomic<int>{0};
    auto bmcRun = std::thread([&] {
        bmcRes = bmc.run();
        finishedID = 1;
        isFinished.notify_all();
    });
    auto kindRun = std::thread([&] {
        kindRes = kind.run();
        finishedID = 2;
        isFinished.notify_all();
    });
    auto lck = std::unique_lock(mux);
    isFinished.wait(lck);
    if (finishedID == 1) {
        logger.log(0, "BMC Finished.");
    } else if (finishedID == 2) {
        logger.log(0, "k-induction finished.");
    } else {
        logger.log(0, "Something wrong.");
    }
    exit(0);
}