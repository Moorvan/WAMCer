//
// Created by Morvan on 2022/11/14.
//

#include <gtest/gtest.h>
#include "core/runner.h"
#include "core/solverFactory.h"

TEST(hwmccTestBench, test1) {
    logger.set_verbosity(0);
    auto f = "path of path.txt";
    // read paths from f
    std::ifstream ifs(f);
    std::string path;
    while (std::getline(ifs, path)) {
        // time it with second
        auto start = std::chrono::steady_clock::now();
        logger.log(defines::logTest, 0, "path = {}", path);
        auto res = Runner::runFBMCWithKInduction(path, BTOR2Encoder::decoder, []() -> auto {
            return SolverFactory::boolectorSolver();
        }, -1, 1, 1, 39);
        logger.log(defines::logTest, 0, "res = {}", res);
        // end time
        logger.log(defines::logTest, 0, "time = {}s", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start).count());
    }
}