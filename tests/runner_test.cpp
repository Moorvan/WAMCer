//
// Created by Morvan on 2022/7/4.
//

#include <gtest/gtest.h>
#include "core/runner.h"
#include "core/solverFactory.h"
#include "cases/counter.h"

using namespace wamcer;

TEST(RunnerTests, runBMC) {
    logger.set_verbosity(1);
    auto path = "../../btors/counter-false.btor";
    Runner::runBMC(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    });
}


TEST(RunnerTests, runBMC0) {
    logger.set_verbosity(1);
    Runner::runBMC("is a transition system", [](std::string path, TransitionSystem &ts, Term &p) {
        counter(ts, p);
    }, []() { return SolverFactory::boolectorSolver(); });
}


TEST(RunnerTests, runBMCWithKInduction) {
    logger.set_verbosity(1);
    auto path = "../../btors/counter-101.btor2";
    Runner::runBMCWithKInduction(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, 1000000);
//    Runner::runBMCWithKInduction(path, 101);
}

TEST(RunnerTests, RunFBMCWithKind) {
    logger.set_verbosity(2);
}