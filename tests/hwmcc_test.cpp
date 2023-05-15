//
// Created by Morvan on 2022/11/14.
//

#include <gtest/gtest.h>
#include "core/runner.h"
#include "core/solverFactory.h"

TEST(hwmccTestBench, test1) {
    logger.set_verbosity(1);
    auto f = "/Users/yuechen/Developer/pycharm-projects/tools/fs/paths.txt";
    // read paths from f
    std::ifstream ifs(f);
    std::string path;
    while (std::getline(ifs, path)) {
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

TEST(GetFiles, getNoConstraints) {
    logger.set_verbosity(1);
    auto f = "/Users/yuechen/Developer/pycharm-projects/tools/fs/sats.txt";
    // read paths from f
    std::ifstream ifs(f);
    std::string path;
    logger.log(0, "path = {}", path);
    while (std::getline(ifs, path)) {
        auto start = std::chrono::steady_clock::now();
//        logger.log(defines::logTest, 0, "path = {}", path);
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        BTOR2Encoder::decoder(path, ts, p);
        for (const auto& kv: ts.constraints()) {
            logger.log(0, "can? {}", kv.second);
        }
//        if (ts.constraints().empty()) {
//            logger.log(defines::logTest, 0, "path = {}", path);
//        }
//        if (ts.only_curr(p)) {
//            logger.log(defines::logTest, 0, "path = {}", path);
//        }
    }
}

TEST(PredCPTesting, 0) {
    auto f = "/Users/yuechen/Developer/pycharm-projects/tools/pono/fine.txt";
    std::ifstream ifs(f);
    std::string path;
    std::ofstream ofs("./res.txt", std::ios::out | std::ios::trunc);
    logger.set_verbosity(1);
    while (std::getline(ifs, path)) {
        auto start = std::chrono::steady_clock::now();
        auto res = Runner::runPredCP(path, BTOR2Encoder::decoder, []() {
            return SolverFactory::boolectorSolver();
        }, [](TransitionSystem &ts, Term &p, AsyncTermSet &preds, SmtSolver &s) {
            auto predsGen = new DirectConstructor(ts, p, preds, s);
            predsGen->generatePreds(1, 1);
        }, 25);
        ASSERT_TRUE(res);
        // write time into res.txt with overwriting
        ofs << path << ": " << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start).count() << "s" << std::endl;
    }
}