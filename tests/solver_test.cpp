//
// Created by Morvan on 2022/6/30.
//

#include <gtest/gtest.h>
#include "smt-switch/bitwuzla_factory.h"
#include "smt-switch/smt.h"
using namespace smt;

TEST(SolverLearningTests, Syntax) {
    auto s = BitwuzlaSolverFactory::create(true);
    auto t = s->make_term(true);
    auto f = s->make_term(false);
    s->assert_formula(s->make_term(And, t, f));
    auto r = s->check_sat();
    printf("%d", r.is_sat());
}
