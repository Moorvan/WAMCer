//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include <core/runner.h>
#include "core/solverFactory.h"
#include <engines/k_induction.h>
#include "engines/pdr.h"
#include "engines/fbmc.h"
#include "smt-switch/boolector_factory.h"
#include "smt-switch/bitwuzla_factory.h"
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
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
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
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
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
    logger.set_verbosity(1);
    auto path = "../../btors/counter-101.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto safeBound = defines::allStepSafe;
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto ts = TransitionSystem(s);
    auto p = Term();
    auto signal_exit = std::promise<void>();
    auto signal = signal_exit.get_future();
    BTOR2Encoder::decoder(path, ts, p);
    auto t = std::thread([&] {
        auto kInd = KInduction(ts, p, safeBound, mux, cv, std::move(signal));
        kInd.run();
    });
    std::this_thread::sleep_for(std::chrono::seconds(1));
    signal_exit.set_value();
    t.join();
//    auto kind = KInduction(ts, p);
//    if (kind.run()) {
//        logger.log(0, "Proved: IS INVARIANT.");
//    } else {
//        logger.log(0, "Unknown.");
//    }
}

TEST(KInductionWithBMC, KindWithBMC) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-true.btor";
    auto s1 = SolverFactory::boolectorSolver();
    auto s2 = SolverFactory::boolectorSolver();
    auto ts1 = TransitionSystem(s1);
    auto ts2 = TransitionSystem(s2);
    auto p1 = BTOR2Encoder(path, ts1).propvec().at(0);
    auto p2 = BTOR2Encoder(path, ts2).propvec().at(0);
    auto safe = int();
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto finish = std::condition_variable();
    auto isFinished = false;
    auto kind = KInduction(ts1, p1, safe, mux, cv);
    auto bmc = BMC(ts2, p2, safe, mux, cv);
    auto wake = std::thread([&] {
        timer::wakeEvery(config::wakeKindCycle, cv, isFinished);
    });
    auto bmcRes = bool();
    auto kindRes = bool();
    auto finishedID = std::atomic<int>{0};
    auto bmcRun = std::thread([&] {
        bmcRes = bmc.run();
        finishedID = 1;
        finish.notify_all();
    });
    auto kindRun = std::thread([&] {
        kindRes = kind.run();
        finishedID = 2;
        finish.notify_all();
    });
    auto lck = std::unique_lock(mux);
    finish.wait(lck);
    isFinished = true;
    if (finishedID == 1) {
        logger.log(0, "BMC Finished.");
    } else if (finishedID == 2) {
        logger.log(0, "k-induction finished.");
    } else {
        logger.log(0, "Something wrong.");
    }
    bmcRun.join();
    kindRun.join();
}

TEST(EasyPDRTests, EasyPDR) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    auto s = BoolectorSolverFactory::create(false);
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto pdr = EasyPDR(ts, p);
    pdr.run();
}

TEST(FBMCTests, FBMCWithKind) {
    logger.set_verbosity(1);
//    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btor2_BM/ret0024_dir.btor2";
    auto path = "../../btors/buffer.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);

    auto preds = UnorderedTermSet();
    auto pred_s = SolverFactory::cvc5Solver();

    auto safeStep = int();
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto to_pred = TermTranslator(pred_s);
    auto fbmc = FBMC(ts, p, preds, safeStep, mux, cv, to_pred);
    fbmc.run(13);
    logger.log(1, "has {} preds.", preds.size());
//    logger.log(1, "safe step is {}", safeStep);
//    logger.log(1, "True preds in 33 steps: ");
//    for (auto v : preds) {
//        logger.log(1, "{}", v);
//    }

    auto simFilter = FilterWithSimulation(path, 30);
    auto eraseTerms = TermVec();
    for (auto pred: preds) {
        if(!simFilter.checkSat(pred)) {
            eraseTerms.push_back(pred);
        }
    }
    logger.log(1, "sim filter erase {} preds.", eraseTerms.size());
    for (auto i : eraseTerms) {
        preds.erase(i);
    }
    logger.log(1, "has {} preds.", preds.size());

    logger.log(1, "test pred pass sim check...");
    if (simFilter.checkSat(p)) {
        logger.log(1, "pred is pass sim check.");
    } else {
        logger.log(1, "pred is not pass sim check.");
    }
    logger.log(1, "has {} preds:", preds.size());

    auto kind_slv = SolverFactory::boolectorSolver();
    auto kind_ts = TransitionSystem(kind_slv);
    auto kind_prop = BTOR2Encoder(path, kind_ts).propvec().at(0);
    auto to_kind_slv = TermTranslator(kind_slv);
    logger.log(1, "old_prop = {}", kind_prop);
    for (auto v: preds) {
        kind_prop = kind_slv->make_term(And, kind_prop, to_kind_slv.transfer_term(v));
    }
    logger.log(1, "new_prop = {}", kind_prop);
    auto kind = KInduction(kind_ts, kind_prop);
    kind.run(10);
}

