//
// Created by Morvan on 2022/7/6.
//

#include <gtest/gtest.h>
#include "cases/counter.h"
#include "core/runner.h"
#include "core/solverFactory.h"
//#include <filesystem>

using namespace wamcer;
using namespace std;

TEST(CasesTests, Counter) {
    logger.set_verbosity(1);
    auto res = Runner::runBMCWithKInduction("is a transition system",
                                            [](std::string path, TransitionSystem &ts, Term &p) {
                                                counter(ts, p);
                                            }, []() {
                return SolverFactory::boolectorSolver();
            }, 101);
//    auto res = Runner::runBMCWithKInduction(counter);
    logger.log(defines::logTest, 0, "res = ", res);
}

TEST(CasesTests, Counter2) {

}

//TEST(FindNoInputCases, noinput) {
//    auto cnt = 0;
//    auto okcnt = 0;
//    auto err = 0;
//    auto f = [&](const string& path) {
//        for (const auto& i: filesystem::directory_iterator(path)) {
//            cnt++;
//            auto fs = i.path().string();
//            auto slv = SolverFactory::boolectorSolver();
//            auto ts = TransitionSystem(slv);
//            try {
//                auto btor2 = BTOR2Encoder(fs, ts);
//                if (btor2.inputsvec().size() == 0) {
//                    logger.log(0, "fs: {}", fs);
//                    okcnt++;
//                }
//            } catch (...) {
//                err++;
//            }
//        }
//    };
//    f("../../btors/hwmcc");
//    f("../../btors/timeout");
//    logger.log(0, "cnt: {}, okcnt: {}, err: {}", cnt, okcnt, err);
//}