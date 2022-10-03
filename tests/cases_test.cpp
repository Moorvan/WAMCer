//
// Created by Morvan on 2022/7/6.
//

#include <gtest/gtest.h>
#include "cases/counter.h"
#include "core/runner.h"
#include "core/solverFactory.h"

using namespace wamcer;

TEST(CasesTests, Counter) {
    logger.set_verbosity(1);
    auto res = Runner::runBMCWithKInduction("is a transition system",[](std::string path, TransitionSystem &ts, Term &p) {
        counter(ts, p);
        }, []() {
        return SolverFactory::boolectorSolver();
        }, 101);
//    auto res = Runner::runBMCWithKInduction(counter);
    logger.log(defines::logTest, 0, "res = ", res);
}