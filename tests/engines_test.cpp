//
// Created by Morvan on 2022/7/3.
//

#include <gtest/gtest.h>
#include <core/runner.h>
#include "core/solverFactory.h"
#include <engines/k_induction.h>
#include "engines/pdr.h"
#include "engines/fbmc.h"
#include "engines/DirectConstructor.h"
#include "engines/BMCChecker.h"
#include "engines/InductionProver.h"
#include "engines/InvConstructor.h"
#include "engines/transitionFolder.h"
#include "smt-switch/boolector_factory.h"
#include "smt-switch/bitwuzla_factory.h"
#include <thread>
#include <chrono>
#include "utils/timer.h"
#include "config.h"
#include "tests/cases/counter.h"

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
    auto step = std::atomic<int>();
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
    auto safeBound = std::atomic<int>(defines::allStepSafe);
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
    auto safe = std::atomic<int>();
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
    logger.set_verbosity(2);
//    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btor2_BM/ret0024_dir.btor2";
    auto path = "../../btors/memory.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);

    auto preds = AsyncTermSet();
    auto pred_s = SolverFactory::cvc5Solver();

    auto gen = DirectConstructor(ts, p, preds, pred_s);
    gen.generatePreds();

    auto safeStep = std::atomic<int>();
    auto fbmc = FBMC(ts, p, preds, safeStep);
    fbmc.run(13);
    logger.log(1, "has {} preds.", preds.size());
    logger.log(1, "safe step is {}", safeStep);

    auto simFilter = FilterWithSimulation(path, 30);
    preds.filter([&](Term t) -> bool {
        return not simFilter.checkSat(t);
    });
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
    kind_prop = preds.reduce([&](Term a, Term b) -> Term {
        return kind_slv->make_term(And, a, to_kind_slv.transfer_term(b));
    }, kind_prop);
    logger.log(1, "new_prop = {}", kind_prop);
    auto kind = KInduction(kind_ts, kind_prop);
    kind.run(10);
}


TEST(PredsCP, bmc) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-101.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMCChecker(ts);
    auto slv = SolverFactory::boolectorSolver();
    auto to_slv = TermTranslator(slv);
    if (bmc.check(3, to_slv.transfer_term(p))) {
        logger.log(0, "bmc pass.");
    } else {
        logger.log(0, "bmc unpass.");
    }

    if (bmc.check(25, to_slv.transfer_term(p))) {
        logger.log(0, "bmc pass.");
    } else {
        logger.log(0, "bmc unpass.");
    }

    if (bmc.check(20, to_slv.transfer_term(s->make_term(Not, p)))) {
        logger.log(0, "bmc pass.");
    } else {
        logger.log(0, "bmc unpass.");
    }
}

TEST(PredsCP, kind) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-101.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto ind = InductionProver(ts, p);
    auto slv = SolverFactory::boolectorSolver();
    auto to_slv = TermTranslator(slv);
    if (ind.prove(25, to_slv.transfer_term(p))) {
        logger.log(0, "ind pass.");
    } else {
        logger.log(0, "ind unpass.");
    }

    if (ind.prove(3, to_slv.transfer_term(p))) {
        logger.log(0, "ind pass.");
    } else {
        logger.log(0, "ind unpass.");
    }

    if (ind.prove(10, to_slv.transfer_term(s->make_term(Not, p)))) {
        logger.log(0, "ind pass.");
    } else {
        logger.log(0, "ind unpass.");
    }

    if (ind.prove(103, to_slv.transfer_term(p))) {
        logger.log(0, "ind pass.");
    } else {
        logger.log(0, "ind unpass.");
    }

    if (ind.prove(10, slv->make_term(false))) {
        logger.log(0, "ind pass.");
    } else {
        logger.log(0, "ind unpass.");
    }
}

TEST(InvConstruc, InvConstructor) {
    auto inv = AsyncTermSet();
    auto path = "../../btors/memory.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder(path, ts, p);
    auto pred_s = SolverFactory::boolectorSolver();
    auto invGen = InvConstructor(ts, p, inv, pred_s);
    invGen.generateInvs();

}

TEST(TSFold, fold) {
    logger.set_verbosity(5);
//    auto path = "../../btors/counter-101.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
//    BTOR2Encoder::decoder(path, ts, p);
    counter(ts, p);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto to_s = TermTranslator(s);
    auto folder = TransitionFolder(ts, folder_slv);
    auto out = Term();
    std::cout << ts.trans()->to_string() << std::endl;
    folder.getNStepTrans(10, out, to_s);
    logger.log("[trans]: ", 1, "ts.trans = {}", ts.trans());
    logger.log("[out]: ", 1, "out = {}", out);
}

TEST(TSFold, foldBench) {
    auto path = "../../btors/memory.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder(path, ts, p);
    auto start0 = std::chrono::steady_clock::now();
    auto unroller = Unroller(ts);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto folder = TransitionFolder(ts, folder_slv);
    auto to_s = TermTranslator(s);
    auto out = Term();
    s->assert_formula(unroller.at_time(ts.init(), 0));
    out = ts.trans();
    folder.getNStepTrans(15, out, to_s);
    s->assert_formula(unroller.at_time(out, 0));
    auto notP = s->make_term(Not, p);
    s->assert_formula(unroller.at_time(notP, 1));
    if (s->check_sat().is_unsat()) {
        logger.log(0, "pass");
    } else {
        logger.log(0, "Not pass");
        exit(1);
    }
    logger.log(0, "time count = {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start0).count());

    auto s1 = SolverFactory::boolectorSolver();
    auto ts1 = TransitionSystem(s1);
    auto p1 = Term();
    BTOR2Encoder::decoder(path, ts1, p1);
    auto bmc = BMCChecker(ts1);
    auto start = std::chrono::steady_clock::now();
    if (bmc.check(15, p1)) {
        logger.log(0, "bmc pass.");
    } else {
        logger.log(0, "bmc unpass.");
    }
    logger.log(0, "time count: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}

TEST(TSFold, foldBench2) {
    auto path = "../../btors/memory.btor2";
    auto start0 = std::chrono::steady_clock::now();
    auto f = [&](int n) {
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        BTOR2Encoder::decoder(path, ts, p);
        auto unroller = Unroller(ts);
        auto folder_slv = SolverFactory::boolectorSolver();
        auto folder = TransitionFolder(ts, folder_slv);
        auto to_s = TermTranslator(s);
        auto out = Term();
        s->assert_formula(unroller.at_time(ts.init(), 0));
        out = ts.trans();
        folder.getNStepTrans(n, out, to_s);
        s->assert_formula(unroller.at_time(out, 0));
        auto notP = s->make_term(Not, p);
        s->assert_formula(unroller.at_time(notP, 1));
        if (s->check_sat().is_unsat()) {
            logger.log(0, "bmc with fold pass in {} step", n);
        } else {
            logger.log(0, "Not pass");
            exit(1);
        }
    };
    for (int i = 1; i <= 20; ++i) {
        f(i);
    }

    logger.log(0, "time count = {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start0).count());

    auto s1 = SolverFactory::boolectorSolver();
    auto ts1 = TransitionSystem(s1);
    auto p1 = Term();
    BTOR2Encoder::decoder(path, ts1, p1);
    auto bmc = BMCChecker(ts1);
    auto start = std::chrono::steady_clock::now();
    auto g = [&](int n) {
        if (bmc.check(n, p1)) {
            logger.log(0, "bmc pass in {} step", n);
        } else {
//            logger.log(0, "bmc unpass.");
            exit(0);
        }
    };
//    for (int i = 15; i > 1; i--) {
//        g(i);
//    }
    for (int i = 1; i <= 20; i++) {
        g(i);
    }

    logger.log(0, "time count: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}



TransitionSystem f() {
    auto slv = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(slv);
    return ts;
}

TEST(PredsCP, parallel) {
    auto path = "../../btors/memory.btor2";
    auto f = [&] {
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        BTOR2Encoder::decoder(path, ts, p);
        auto bmc = BMCChecker(ts);
        if (bmc.check(10, p)) {
            logger.log(0, "bmc pass.");
        } else {
            logger.log(0, "bmc unpass.");
            exit(1);
        }
    };
    for (auto i = 0; i < 100; i++) {
        f();
    }
}

// bug?
TEST(TestBMC, bmc) {
    auto path = "../../btors/memory.btor2";
    auto f = [&] {
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto unroller = Unroller(ts);
        auto p = Term();
        BTOR2Encoder::decoder(path, ts, p);
        s->assert_formula(unroller.at_time(ts.init(), 0));
        s->assert_formula(unroller.at_time(ts.trans(), 0));
        auto notP = s->make_term(Not, p);
        s->assert_formula(unroller.at_time(notP, 1));
        if (s->check_sat().is_unsat()) {
            logger.log(0, "bmc pass.");
        } else {
            logger.log(0, "bmc unpass.");
            exit(1);
        }
    };
    for (auto i = 0; i < 100; i++) {
        f();
    }
}

TEST(TestBMC, bmcBug) {
    auto path = "../../btors/memory.btor2";
    auto f = [&] {
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        BTOR2Encoder::decoder(path, ts, p);
        s->assert_formula(ts.init());
        s->assert_formula(ts.trans());
        auto notP = s->make_term(Not, p);
        s->assert_formula(ts.next(notP));
        if (s->check_sat().is_unsat()) {
            logger.log(0, "bmc pass.");
        } else {
            logger.log(0, "bmc unpass.");
            exit(1);
        }
    };
    for (auto i = 0; i < 100; i++) {
        f();
    }
}