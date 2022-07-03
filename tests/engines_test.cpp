//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include <core/runner.h>
#include <engines/k_induction.h>
#include <thread>
#include <chrono>

using namespace wamcer;

TEST(BMCTests, SafeStep) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    auto step = int();
    logger.log(0, "file: {}.", path);
    logger.log(0, "BMC running...");
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMC(ts, p, step);
    auto t = std::thread([&] {
        bmc.run(-1);
    });
    while (true) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);
        logger.log(0, "safe step: {}", step);
    }
    t.join();
}

TEST(KInductionTests, SingleKInd) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-101.btor2";
    auto step = 10;
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto kind = KInduction(ts, p, step);
    if (kind.run()) {
        logger.log(0, "Proved: IS INVARIANT.");
    } else {
        logger.log(0, "Unknown.");
    }
}