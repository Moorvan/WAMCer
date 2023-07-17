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
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n2_w8_d128_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMCChecker(ts);
    auto slv = SolverFactory::boolectorSolver();
    auto to_slv = TermTranslator(slv);
    for (auto i = 0; i < 20; i++) {
        if (bmc.check(i + 1, to_slv.transfer_term(p))) {
            logger.log(0, "bmc pass at {}", i + 1);
        } else {
            logger.log(0, "bmc unpass.");
        }
    }
//    if (bmc.check(20, to_slv.transfer_term(p))) {
//        logger.log(0, "bmc pass.");
//    } else {
//        logger.log(0, "bmc unpass.");
//    }
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
    logger.set_verbosity(2);
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
    folder.getNStepTrans(2, out, to_s);
    logger.log("[trans]: ", 1, "ts.trans = {}", ts.trans());
    logger.log("[out]: ", 1, "out = {}", out);
}

TEST(TSFold, cntFoldBMC) {
    logger.set_verbosity(0);
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
//    BTOR2Encoder::decoder(path, ts, p);
    counter(ts, p);
//    logger.log(0, "input size: {}", ts.inputvars().size());
//    logger.log(0, "state size: {}", ts.statevars().size());
//    logger.log(0, "constraint size: {}", ts.constraints().size());
//    logger.log(0, "updates size: {}", ts.state_updates().size());
//    ts.convert_no_updates_to_inputs();
//    logger.log(0, "input size: {}", ts.inputvars().size());
//    logger.log(0, "state size: {}", ts.statevars().size());
//    logger.log(0, "constraint size: {}", ts.constraints().size());
//    logger.log(0, "updates size: {}", ts.state_updates().size());
    auto folder_slv = SolverFactory::boolectorSolver();
    auto to_s = TermTranslator(s);
    auto folder = TransitionFolder(ts, folder_slv);
    auto out = Term();
    std::cout << ts.trans()->to_string() << std::endl;
    auto unroller = Unroller(ts);
    auto f = [&](int n) {
        s->push();
        s->assert_formula(unroller.at_time(ts.init(), 0));
        out = ts.trans();
        folder.getNStepTrans(n, out, to_s);
//        logger.log(0, "trans_{} = {}", n, out);
//        logger.log(0, "checking at {} step", n);
        s->assert_formula(unroller.at_time(out, 0));
        auto notP = s->make_term(Not, p);
        s->assert_formula(unroller.at_time(notP, 1));
        if (s->check_sat().is_unsat()) {
            logger.log(0, "bmc with fold pass in {} step", n);
        } else {
            logger.log(0, "Not pass in {} step", n);
        }
        s->pop();
    };
    for (int i = 1; i <= 30; ++i) {
        f(i);
    }
}

TEST(TSFold, BMCcase) {
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    ts.convert_no_updates_to_inputs();
    auto p = Term();
    counter(ts, p);
    logger.log(0, "constraints size: {}", ts.constraints().size());
    auto bmc = BMCChecker(ts);
    auto f = [&](int i) {
        if (bmc.check(i, p)) {
            logger.log(0, "bmc pass in {} step.", i);
        } else {
            logger.log(0, "bmc unpass at {} step", i);
        }
    };
    for (auto i = 0; i < 30; i++) {
        f(i);
    }
}


TEST(FSFold, fold2) {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder(path, ts, p);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto folder = TransitionFolder(ts, folder_slv);
    auto out = Term();
    auto terms = TermVec();
    auto f = [&](int n, const Term &t) {
        auto to_s = TermTranslator(s);
        terms.push_back(to_s.transfer_term(t));
    };
    folder.foldToNStep(30, f);
//    folder.foldToNStep(103, f);
}

TEST(TSFold, foldTest) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder_with_constraint(path, ts, p);
    auto init = ts.init();
    auto unroller = Unroller(ts);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto folder = TransitionFolder(ts, folder_slv);
    auto to_s = TermTranslator(s);
    auto out = Term();
    s->assert_formula(unroller.at_time(init, 0));
    auto f = [&](int t) {
        s->push();
        folder.getNStepTrans2(t, out, to_s);
        auto notP = s->make_term(Not, p);
        s->assert_formula(unroller.at_time(notP, t));
        s->assert_formula(out);
        if (s->check_sat().is_unsat()) {
            logger.log(0, "pass in {} step", t);
        } else {
            logger.log(0, "Not pass in {} step", t);
        }
        s->pop();
    };
//    f(2);
//    f(20);
    for (int i = 1; i <= 20; ++i) {
        f(i);
    }
}

TEST(Direct, call) {
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n3_w8_d128_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder_with_constraint(path, ts, p);
    auto init = ts.init();
    auto unroller = Unroller(ts);
    s->assert_formula(unroller.at_time(init, 0));
    auto f = [&](int t) {
        for (int i = 0; i < t; i++) {
            s->assert_formula(unroller.at_time(ts.trans(), i));
        }
        auto not_p = s->make_term(Not, p);
        assert(t > 0);
        auto not_p_s = s->make_term(false);
        for (int i = 1; i <= t; i++) {
            not_p_s = s->make_term(Or, not_p_s, unroller.at_time(not_p, i));
        }
        s->assert_formula(not_p_s);
        if (s->check_sat().is_unsat()) {
            logger.log(0, "pass in {} step", t);
        } else {
            logger.log(0, "Not pass in {} step", t);
        }
    };
    f(26);
}


TEST(AAA, BBB) {
    auto slv = SolverFactory::boolectorSolver();
    auto f = [&] -> Term {
        return slv->make_term(true);
    };

    auto b = f();
    b = slv->make_term(And, b, slv->make_term(false));
    logger.log(0, "a = {}", f());
    logger.log(0, "b = {}", b);

    auto a = std::unordered_map<int, int>();
    a[1] = 2;
    logger.log(0, "a = {}", a[1]);
    logger.log(0, "a = {}", a[0]);
}

TEST(TSFold, BMCs) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder(path, ts, p);
    auto bmc = BMCChecker(ts);
    auto f = [&](int i) {
        if (bmc.check(i, p)) {
            logger.log(0, "bmc pass in {} step.", i);
        } else {
            logger.log(0, "bmc unpass at {} step", i);
        }
    };
    // I0 and T01 and T12 ... T19-20 and not P20
    for (int i = 1; i <= 20; i++) {
        f(i);
    }

}

TEST(TSFold, foldBench) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder_with_constraint(path, ts, p);
//    BTOR2Encoder::decoder(path, ts, p);
//    auto encoder = BTOR2Encoder(path, ts, true);
//    p = encoder.prop();
    auto init = ts.init();
//    init = ts.make_term(And, init, encoder.constraint());
//    logger.log(0, "input size: {}", ts.inputvars().size());
//    logger.log(0, "state size: {}", ts.statevars().size());
//    logger.log(0, "constraints size: {}", ts.constraints().size());
//    auto start0 = std::chrono::steady_clock::now();
    auto unroller = Unroller(ts);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto folder = TransitionFolder(ts, folder_slv);
    auto to_s = TermTranslator(s);
    auto out = Term();
    s->assert_formula(unroller.at_time(init, 0));
//    out = ts.trans();
    folder.getNStepTrans(3, out, to_s);
    logger.log(0, "construct trans ok.");
    s->assert_formula(unroller.at_time(out, 0));
    auto notP = s->make_term(Not, p);
    s->assert_formula(unroller.at_time(notP, 1));
    if (s->check_sat().is_unsat()) {
        logger.log(0, "pass");
    } else {
        logger.log(0, "Not pass");
//        exit(1);
    }
//    logger.log(0, "time count = {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(
//            std::chrono::steady_clock::now() - start0).count());

//    auto s1 = SolverFactory::boolectorSolver();
//    auto ts1 = TransitionSystem(s1);
//    auto p1 = Term();
//    BTOR2Encoder::decoder(path, ts1, p1);
//    auto bmc = BMCChecker(ts1);
//    auto start = std::chrono::steady_clock::now();
//    if (bmc.check(20, p1)) {
//        logger.log(0, "bmc pass.");
//    } else {
//        logger.log(0, "bmc unpass.");
//    }
//    logger.log(0, "time count: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}

//auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019A/picorv32_mutAY_nomem-p4.btor";
//auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019A/picorv32_mutBX_nomem-p5.btor";

TEST(TSFold, foldBench2) {
//    logger.set_verbosity(1);
//    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/wolf/2019A/picorv32_mutBY_nomem-p4.btor";
    auto start0 = std::chrono::steady_clock::now();
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
//    BTOR2Encoder::decode_without_constraint(path, ts, p);
//    ts.convert_no_updates_to_inputs();

    // consider without constraint
    auto encoder = BTOR2Encoder(path, ts, true);
    p = encoder.prop();
    logger.log(0, "p = {}", p);
    ts.add_init(encoder.constraint());
    p = ts.make_term(Implies, encoder.constraint(), p);

//    ts.convert_no_updates_to_inputs();
//    logger.log(0, "input size: {}", ts.inputvars().size());
//    logger.log(0, "state size: {}", ts.statevars().size());
//    logger.log(0, "constraint size: {}", ts.constraints().size());
//    logger.log(0, "updates size: {}", ts.state_updates().size());
    auto unroller = Unroller(ts);
    auto folder_slv = SolverFactory::boolectorSolver();
    auto folder = TransitionFolder(ts, folder_slv);
    auto to_s = TermTranslator(s);

//    s->push();
//    {
////        if (ts.only_curr(p)) {
////            logger.log(0, "only curr");
////        } else {
////            if (ts.no_next(p)) {
////                logger.log(0, "no next");
////            } else {
////                logger.log(0, "has next");
////            }
////        } // statevars_, &inputvars_
////        if (ts.only_curr(ts.init())) {
////            logger.log(0, "only curr");
////        } else {
////            logger.log(0, "has next");
////        }
//        s->assert_formula(unroller.at_time(ts.init(), 0));
//        s->assert_formula(unroller.at_time(ts.trans(), 0));
//        s->assert_formula(unroller.at_time(ts.trans(), 1));
//        s->assert_formula(unroller.at_time(ts.trans(), 2));
//        s->assert_formula(unroller.at_time(ts.trans(), 3));
//        s->assert_formula(unroller.at_time(ts.trans(), 4));
//        p = s->make_term(Implies, encoder.constraint(), p);
//        auto notP = s->make_term(Not, p);
//        s->assert_formula(unroller.at_time(notP, 5));
//        s->assert_formula(ts.make_term(Not, p));
//        if (s->check_sat().is_unsat()) {
//            logger.log(0, "is unsat");
//        } else {
//            logger.log(0, "sat");
//            exit(1);
//        }
//    }
//    s->pop();
//    exit(0);

    auto out = Term();
    auto bmc1 = BMCChecker(ts);
    for (auto i = 1; i < 22; i++) {
        out = Term();
        folder.getNStepTrans(i, out, to_s);
        if (bmc1.check(out, p)) {
            logger.log(0, "bmc with fold pass in {} step", i);
        } else {
            logger.log(0, "Not pass at {} step", i);
        }
    }
//    s->assert_formula(unroller.at_time(ts.init(), 0));
//    auto f = [&](int n) {
//        out = Term();
//        folder.getNStepTrans(n, out, to_s);
//        if (bmc1.check(out, p)) {
//            logger.log(0, "bmc with fold pass in {} step", n);
//        } else {
//            logger.log(0, "Not pass at {} step", n);
//        }
//        s->assert_formula(unroller.at_time(out, 0));
//        auto notP = s->make_term(Not, p);
//        s->assert_formula(unroller.at_time(notP, 1));
//        if (s->check_sat().is_unsat()) {
//        } else {
//            exit(1);
//        }
//        s->pop();
//    };
//    {
//        auto slv = SolverFactory::boolectorSolver();
//        auto ts = TransitionSystem(slv);
//        auto p = Term();
//        BTOR2Encoder::decoder(path, ts, p);
//        auto checker = BMCChecker(ts);
//        if (checker.check(0, p)) {
//            logger.log(defines::logBMCWithFolderRunner, 0, "safe at 0 step");
//        } else {
//            logger.log(defines::logBMCWithFolderRunner, 0, "unsafe at 0 step");
//            exit(0);
//        }
//    }
//    for (int i = 1; i <= 20; ++i) {
//        f(i);
//    }

//    logger.log(0, "time count = {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(
//            std::chrono::steady_clock::now() - start0).count());

//    auto s1 = SolverFactory::boolectorSolver();
//    auto ts1 = TransitionSystem(s1);
//    auto p1 = Term();
//    BTOR2Encoder::decoder(path, ts1, p1);
//    auto bmc = BMCChecker(ts1);
//    auto start = std::chrono::steady_clock::now();
//    auto g = [&](int n) {
//        if (bmc.check(n, p1)) {
//            logger.log(0, "bmc pass in {} step", n);
//        } else {
//            logger.log(0, "bmc unpass.");
//            exit(0);
//        }
//    };
//    for (int i = 15; i > 1; i--) {
//        g(i);
//    }
//    for (int i = 1; i <= 30; i++) {
//        g(i);
//    }

//    logger.log(0, "time count: {} ms",
//               std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count());
}

TEST(TSFold, BMC) {
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n4_w16_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
//    BTOR2Encoder::decoder(path, ts, p);
//    logger.log(0, "prop: {}", p);
    auto encoder = BTOR2Encoder(path, ts);
    p = encoder.prop();
//    ts.add_init(encoder.constraint());
//    p = ts.make_term(Implies, encoder.constraint(), p);
//    ts.convert_no_updates_to_inputs();
    auto bmc = BMCChecker(ts);
//    logger.log(0, "prop: {}", p);
    logger.log(0, "constraints size: {}", ts.constraints().size());
    auto f = [&](int i) {
        if (bmc.check(i, p)) {
            logger.log(0, "bmc pass in {} step.", i);
        } else {
            logger.log(0, "bmc unpass at {} step", i);
            exit(1);
        }
    };
    for (auto i = 0; i < 100; i++) {
        f(i);
    }
}

TEST(TSFold, BMC_in_constraints) {
    auto path = "/Users/yuechen/Documents/study/btors/hwmccs/hwmcc20/btor2/bv/2019/mann/data-integrity/unsafe/arbitrated_top_n4_w16_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder_with_constraint(path, ts, p);
//    ts.convert_no_updates_to_inputs();
    auto bmc = BMCChecker(ts);
//    logger.log(0, "prop: {}", p);
    logger.log(0, "constraints size: {}", ts.constraints().size());
    auto f = [&](int i) {
        if (bmc.check(i, p)) {
            logger.log(0, "bmc pass in {} step.", i);
        } else {
            logger.log(0, "bmc unpass at {} step", i);
            exit(1);
        }
    };
    for (auto i = 0; i < 50; i++) {
        f(i);
    }
}

TEST(TSFold, caseError) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/hwmcc/arbitrated_top_n3_w8_d16_e0.btor2";
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = Term();
    BTOR2Encoder::decoder(path, ts, p);
    logger.log(0, "input size: {}", ts.inputvars().size());
    logger.log(0, "state size: {}", ts.statevars().size());
    logger.log(0, "constraints: {}", ts.constraints().size());
//    logger.log(0, "init: {}", ts.init());
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