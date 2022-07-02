//
// Created by Morvan on 2022/6/30.
//


#include <iostream>
#include <gtest/gtest.h>
#include "smt-switch/bitwuzla_factory.h"
#include "smt-switch/smt.h"
#include "utils/logger.h"
#include "core/ts.h"
#include "frontends/btor2_encoder.h"
#include "core/unroller.h"

using namespace smt;
using namespace std;
using namespace wamcer;

TEST(SolverLearningTests, Syntax) {
    auto s = BitwuzlaSolverFactory::create(false);
    s->set_opt("incremental", "true");
    s->set_opt("produce-models", "true");
    auto bv8Sort = s->make_sort(BV, 8);
    auto funcSort = s->make_sort(FUNCTION, {bv8Sort, bv8Sort, bv8Sort});
    auto f = s->make_symbol("f", funcSort);
    auto x = s->make_symbol("x", bv8Sort);
    auto t1 = s->make_term(Equal, s->make_term(Apply, {f, s->make_term(1, bv8Sort), x}), s->make_term(1, bv8Sort));
    auto t2 = s->make_term(Equal, s->make_term(Apply, {f, s->make_term(1, bv8Sort), s->make_term(1, bv8Sort)}), s->make_term(2, bv8Sort));
    auto t3 = s->make_term(Equal, x, s->make_term(1, bv8Sort));
    s->assert_formula(s->make_term(And, {t1, t2, t3}));
    auto res = s->check_sat();
    if (res.is_sat()) {
        cout << s->get_value(f) << endl;
    } else {
        logger.log(0, "unsat");
    }
}

TEST(SolverLearningTests, Rewrite) {
    logger.log(0, "log testing..");
}

TEST(Btor2Tests, Btor2Parser) {
    auto ts = TransitionSystem();
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/counter-false.btor";
    BTOR2Encoder be(path, ts);
    logger.log(0, ts.trans()->to_string());
    auto unroller = Unroller(ts);
    auto init0 = unroller.at_time(ts.init(), 0);
    logger.log(0, init0->to_string());
    auto solver = ts.solver();
    solver->assert_formula(init0);
}
