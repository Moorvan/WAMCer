//
// Created by Morvan on 2022/7/6.
//

#include "counter.h"

namespace wamcer {

    void counter(TransitionSystem &ts, Term &property) {
        auto bv8 = ts.make_sort(BV, 8);
        auto x = ts.make_statevar("x", bv8);
        auto i = ts.make_inputvar("i", bv8);
        auto y = ts.make_statevar("y", bv8);
        auto inc = ts.make_term(BVAdd, x, y);
        auto cond = ts.make_term(Equal, x, ts.make_term(50, bv8));
        auto zero = ts.make_term(0, bv8);
        auto fx = ts.make_term(Ite, cond, zero, inc);
        ts.assign_next(x, fx);
        ts.assign_next(y, i);
        ts.set_init(ts.make_term(Equal, x, zero));
        auto constraint = ts.make_term(BVUle, y, ts.make_term(2, bv8));
        ts.add_constraint(constraint);
        property = ts.make_term(BVUlt, x, ts.make_term(50, bv8));
//        ts.add_init(constraint);
//        property = ts.make_term(Implies, constraint, property);
    }
}
