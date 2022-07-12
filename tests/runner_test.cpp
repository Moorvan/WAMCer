//
// Created by Morvan on 2022/7/4.
//

#include <gtest/gtest.h>
#include <core/runner.h>

using namespace wamcer;

TEST(RunnerTests, RunBMC) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-false.btor";
    Runner::runBMC(path);
}

TEST(RunnerTests, RunBMCWithKind) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-101.btor2";
    Runner::runBMCWithKInduction(path, 104);
}