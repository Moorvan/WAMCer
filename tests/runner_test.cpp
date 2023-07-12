//
// Created by Morvan on 2022/7/4.
//

#include <gtest/gtest.h>
#include "core/runner.h"
#include "core/solverFactory.h"
#include "cases/counter.h"
//#include <filesystem>

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
    Runner::runBMC("is a transition system", [](const std::string &path, TransitionSystem &ts, Term &p) {
        counter(ts, p);
    }, []() { return SolverFactory::boolectorSolver(); });
}


TEST(RunnerTests, runBMCWithKInduction) {
    logger.set_verbosity(1);
    auto path = "../../btors/memory.btor2";
    Runner::runBMCWithKInduction(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, 102);
}

TEST(RunnerTests, runFBMCWithKInductionTrue) {
    logger.set_verbosity(1);
    auto path = "../../btors/memory.btor2";
    auto res = Runner::runFBMCWithKInduction(path, BTOR2Encoder::decoder, []() -> auto {
        return SolverFactory::boolectorSolver();
    }, -1, 1, 1, 3);
    ASSERT_TRUE(res);
    logger.log(defines::logTest, 0, "res = {}", res);
}

TEST(RunnerTests, runBMCWithFolder) {
    logger.set_verbosity(2);
//    auto path = "../../btors/memory.btor2";
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n3_w8_d16_e0.btor2";

//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019A/picorv32_mutBY_nomem-p4.btor";
    auto res = Runner::runBMCWithFolder(path, BTOR2Encoder::decoder_with_constraint, []() {
        return SolverFactory::boolectorSolver();
    }, 21, 8, 2);
    ASSERT_TRUE(res);
}

TEST(RunnerTests, runBMCs) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n4_w16_d16_e0.btor2";
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n3_w8_d128_e0.btor2";
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019C/dspfilters_fastfir_second-p07.btor";
//    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/cpu/testbench.btor2";
    auto res = Runner::runBMCs(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, 20, 5);
//    ASSERT_FALSE(res);
}

TEST(RunnerTests, runBMCsInFile) {
    using namespace std;
    logger.set_verbosity(2);
    auto btors = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/files.txt";
    // read lines in file btors
    ifstream ifs(btors);
    string line;
    while (getline(ifs, line)) {
        auto res = Runner::runBMCs(line, BTOR2Encoder::decoder, []() {
            return SolverFactory::boolectorSolver();
        }, 15, 5);
    }
    ifs.close();
}

TEST(RunnerTest, runPredCP) {
    logger.set_verbosity(1);
//    auto path = "../../btors/memory.btor2";
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019C/vgasim_imgfifo-p082.btor";
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/goel/industry/mul7/mul7.btor2";
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019C/vgasim_imgfifo-p089.btor";
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2018D/zipcpu-zipmmu-p48.btor";
    auto res = Runner::runPredCP(path, BTOR2Encoder::decoder_with_constraint, []() {
        return SolverFactory::boolectorSolver();
    }, [](TransitionSystem &ts, Term &p, AsyncTermSet &preds, SmtSolver &s) {
        auto predsGen = new DirectConstructor(ts, p, preds, s);
        predsGen->generatePreds(1, 1);
    }, 25);
    ASSERT_TRUE(res);
}
