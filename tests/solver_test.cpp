//
// Created by Morvan on 2022/6/30.
//


#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include "smt-switch/bitwuzla_factory.h"
#include "smt-switch/boolector_factory.h"
#include "smt-switch/z3_factory.h"
#include "smt-switch/cvc5_factory.h"
#include "smt-switch/smt.h"
#include "utils/logger.h"
#include "core/ts.h"
#include "core/solverFactory.h"
#include "frontends/btor2_encoder.h"
#include "core/unroller.h"
#include "core/runner.h"

using namespace smt;
using namespace std;
using namespace wamcer;

TEST(Solvers, smts) {
    auto bitwuzla = BitwuzlaSolverFactory::create(true);
    auto sort0 = bitwuzla->make_sort(BV, 8);
    auto x0 = bitwuzla->make_symbol("x", sort0);
    auto y0 = bitwuzla->make_symbol("y", sort0);
    auto t0 = bitwuzla->make_term(Equal, x0, y0);
    bitwuzla->assert_formula(t0);
    if (bitwuzla->check_sat().is_sat()) {
        logger.log(defines::logTest, 0, "sat");
    }
    auto btor = BoolectorSolverFactory::create(true);
    auto sort1 = btor->make_sort(BV, 8);
    auto x1 = btor->make_symbol("x", sort1);
    auto y1 = btor->make_symbol("y", sort1);
    auto t1 = btor->make_term(Equal, x1, y1);
    btor->assert_formula(t1);
    if (btor->check_sat().is_sat()) {
        logger.log(defines::logTest, 0, "sat");
    }
    auto z3 = Z3SolverFactory::create(true);
    auto sort2 = z3->make_sort(BV, 8);
    auto x2 = z3->make_symbol("x", sort2);
    auto y2 = z3->make_symbol("y", sort2);
    auto t2 = z3->make_term(Equal, x2, y2);
    z3->assert_formula(t2);
    if (z3->check_sat().is_sat()) {
        logger.log(defines::logTest, 0, "sat");
    }
    auto cvc5 = Cvc5SolverFactory::create(true);
    auto sort3 = cvc5->make_sort(BV, 8);
    auto x3 = cvc5->make_symbol("x", sort3);
    auto y3 = cvc5->make_symbol("y", sort3);
    auto t3 = cvc5->make_term(Equal, x3, y3);
    cvc5->assert_formula(t3);
    if (cvc5->check_sat().is_sat()) {
        logger.log(defines::logTest, 0, "sat");
    }
}

TEST(Solvers, rewrite) {
    auto btor = BoolectorSolverFactory::create(true);
    auto sort1 = btor->make_sort(BV, 8);
    auto x1 = btor->make_symbol("x", sort1);
    auto y1 = btor->make_symbol("y", sort1);
    auto t1 = btor->make_term(Equal, x1, y1);
    btor->assert_formula(t1);
    if (btor->check_sat().is_sat()) {
        logger.log(defines::logTest, 0, "sat");
    }
    logger.log(defines::logTest, 0, "{}", t1);
}

TEST(SolverLearningTests, Syntax) {
    auto s = BitwuzlaSolverFactory::create(false);
    s->set_opt("incremental", "true");
    s->set_opt("produce-models", "true");
    auto bv8Sort = s->make_sort(BV, 1024);
    auto funcSort = s->make_sort(FUNCTION, {bv8Sort, bv8Sort});
    auto mem = s->make_symbol("mem", funcSort);
    auto x = s->make_symbol("x", bv8Sort);
    auto cond = s->make_term(Equal, x, s->make_term(0, bv8Sort));
    auto t1 = s->make_term(Equal, s->make_term(Apply, {mem, x}), x);

    auto tt1 = s->make_term(Equal, s->make_term(Apply, mem, x), s->make_term(2, bv8Sort));
//    s->assert_formula(t1);
//    s->assert_formula(tt1);

    auto array = s->make_sort(ARRAY, bv8Sort, bv8Sort);
    auto memArray = s->make_symbol("mem2", array);
    auto t11 = s->make_term(Store, memArray, x, s->make_term(10, bv8Sort));
    auto ite = s->make_term(Ite, cond, memArray, t11);
//    auto ite2 = s->make_term(Ite, cond, mem, t1);
    auto mem2 = s->make_term(Store, memArray, x, x);
    auto t2 = s->make_term(Equal, s->make_term(Select, mem2, x), s->make_term(2, bv8Sort));
    s->assert_formula(t2);
    auto res = s->check_sat();
    if (res.is_sat()) {
        logger.log(defines::logTest, 0, "{}", s->get_value(mem));
        logger.log(defines::logTest, 0, "{}", s->get_value(x));
    } else {
        logger.log(defines::logTest, 0, "Unsatisfiable");
    }
//    auto f = s->make_symbol("f", funcSort);
//    auto t1 = s->make_term(Equal, s->make_term(Apply, {f, s->make_term(1, bv8Sort), x}), s->make_term(1, bv8Sort));
//    auto t2 = s->make_term(Equal, s->make_term(Apply, {f, s->make_term(1, bv8Sort), s->make_term(1, bv8Sort)}),
//                           s->make_term(2, bv8Sort));
//    auto t3 = s->make_term(Equal, x, s->make_term(1, bv8Sort));
//    s->assert_formula(s->make_term(And, {t1, t2, t3}));
//    auto res = s->check_sat();
//    if (res.is_sat()) {
//        cout << s->get_value(f) << endl;
//    } else {
//        logger.log(0, "unsat");
//    }
}


TEST(Btor2Tests, Btor2Parser) {
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-false.btor";
    BTOR2Encoder be(path, ts);
    logger.log(0, ts.trans()->to_string());
    auto unroller = Unroller(ts);
    auto init0 = unroller.at_time(ts.init(), 0);
    logger.log(0, init0->to_string());
    auto solver = ts.solver();
    solver->assert_formula(init0);
}

TEST(SolverTests, Ints) {
    auto slv = SolverFactory::boolectorSolver();
    auto x = slv->make_symbol("x", slv->make_sort(INT));
    // x > 5
    auto t1 = slv->make_term(Gt, x, slv->make_term(5, slv->make_sort(INT)));
    // x < 10
    auto t2 = slv->make_term(Lt, x, slv->make_term(10, slv->make_sort(INT)));
    auto res = slv->check_sat_assuming({t1, t2});
    if (res.is_sat()) {
        logger.log(0, "sat");
        logger.log(0, "x = {}", slv->get_value(x));

    } else {
        logger.log(0, "unsat");
        auto unsatCore = UnorderedTermSet();
        slv->get_unsat_assumptions(unsatCore);
        for (auto &t: unsatCore) {
            logger.log(0, "unsat core: {}", t);
        }
    }
}

TEST(MultiThreadTests, KInduction) {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
//    Runner::runBMC(path);
    auto s = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s);
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto safe = std::atomic<int>(10);
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto kind = KInduction(ts, p, safe, mux, cv);
    std::thread t([&] {
        kind.run();
    });

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(2s);
    logger.log(defines::logTest, 0, "sleep over.");
    safe++;
    cv.notify_all();
    logger.log(defines::logTest, 0, "after notify");
    t.join();
}

TEST(MultiThreadTests, NotifyWaitLearning) {
    auto mux = std::mutex();
    auto cv = std::condition_variable();

    auto f = [&]() {
        logger.log(0, "f()");
        auto lck = std::unique_lock<std::mutex>(mux);
        cv.wait(lck);
        logger.log(0, "wake up!");
    };

    auto g = [&]() {
        logger.log(0, "g()");
//        auto lck = std::unique_lock<std::mutex>(mux);
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(2s);
        logger.log(0, "WAKE~");
        cv.notify_all();
    };

    auto t0 = std::thread(f);
    auto t00 = std::thread(f);
    auto t1 = std::thread(g);

    t0.join();
    t00.join();
    t1.join();
}


TEST(MultiThreadTests, WaitAndJoin) {
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto finished = std::atomic<int>{-1};
    auto t1 = std::thread([&] {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        finished = 0;
        cv.notify_all();
    });
    auto t2 = std::thread([&] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        finished = 1;
        cv.notify_all();
    });
    auto lck = std::unique_lock(mux);
    cv.wait(lck);
    if (finished == 1) {
        logger.log(0, "t2 finished");
    }
    if (finished == 0) {
        logger.log(0, "t1 finished");
    }
    exit(0);
}

TEST(SolverLearningTests, Simplify) {
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    auto s0 = SolverFactory::boolectorSolver();
    auto ts = TransitionSystem(s0);
    auto s = ts.solver();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto unroller = Unroller(ts);
    auto init0 = unroller.at_time(ts.init(), 0);
    s->assert_formula(init0);
    auto b = s->make_term(Not, p);
    auto bad = unroller.at_time(b, 0);
    for (int i = 0; i < 2; i++) {
        s->assert_formula(unroller.at_time(ts.trans(), i));
        bad = s->make_term(Or, bad, unroller.at_time(b, i + 1));
    }
//    s->assert_formula(bad);
//    auto res = s->check_sat(bad);
    auto res = s->check_sat_assuming({bad});
    logger.log(defines::logTest, 0, "check result: {}", res);
    auto usc = UnorderedTermSet();
    s->get_unsat_assumptions(usc);
    logger.log(defines::logTest, 0, "usc: ");
    for (const auto &t: usc) {
        logger.log(0, "t: {}", t);
    }
}

TEST(SolverTests, UnsatCore) {
    auto s = BitwuzlaSolverFactory::create(false);
    s->set_opt("incremental", "true");
    s->set_opt("produce-models", "true");
    auto a = s->make_symbol("a", s->make_sort(BOOL));
    auto b = s->make_symbol("b", s->make_sort(BOOL));
//    s->assert_formula(a);
//    s->assert_formula(s->make_term(Not, a));
    auto res = s->check_sat_assuming({a, b, s->make_term(Not, a)});
    auto out = UnorderedTermSet();
    if (res.is_unsat()) {
        s->get_unsat_assumptions(out);
        for (auto t: out) {
            logger.log(0, "{}", t);
        }
    }
}

TEST(TranslateTests, Trans) {
    auto s = SolverFactory::boolectorSolver();
    auto s1 = SolverFactory::boolectorSolver();
    auto s2 = SolverFactory::boolectorSolver();
    auto trans = TermTranslator(s);
    auto t1 = s1->make_symbol("a", s1->make_sort(BOOL));
    auto t2 = s2->make_symbol("b", s2->make_sort(BOOL));
    auto st1 = trans.transfer_term(t1);
    auto st2 = trans.transfer_term(t2);
    auto t1andt2 = s->make_term(And, st1, st2);
}

TEST(SolverTests, Translator) {
    auto s = SolverFactory::boolectorSolver();
    auto s1 = SolverFactory::boolectorSolver();
    auto t = s->make_symbol("a", s->make_sort(BOOL));
    auto to_s1 = TermTranslator(s1);
    auto to_s = TermTranslator(s);
    s->assert_formula(t);
    auto t1 = s->make_term(Not, t);
    t1 = to_s1.transfer_term(t1);
    t1 = to_s.transfer_term(t1);
    s->assert_formula(t1);
    if (s->check_sat().is_sat()) {
        logger.log(0, "false");
    } else {
        logger.log(0, "true");
    }
}

TEST(SolverTests, Symbols) {
    auto s = SolverFactory::boolectorSolver();
    if (s->check_sat().is_sat()) {
        logger.log(0, "false");
    } else {
        logger.log(0, "true");
    }
    auto set = UnorderedTermSet();
    set.insert(s->make_symbol("a", s->make_sort(BOOL)));
    set.insert(s->make_symbol("b", s->make_sort(BOOL)));
    logger.log(0, "set size: {}", set.size());
    auto set1 = set;
    set.insert(s->make_symbol("c", s->make_sort(BOOL)));
    logger.log(0, "set size: {}", set.size());
    logger.log(0, "set1 size: {}", set1.size());
    set1.insert(s->make_symbol("d", s->make_sort(BOOL)));
    set1.insert(s->make_symbol("e", s->make_sort(BOOL)));
    logger.log(0, "set size: {}", set.size());
    logger.log(0, "set1 size: {}", set1.size());
}