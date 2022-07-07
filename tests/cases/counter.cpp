//
// Created by Morvan on 2022/7/6.
//

#include "counter.h"

namespace wamcer {

    void counter(TransitionSystem &ts, Term &property) {
        auto bv8 = ts.make_sort(BV, 8);
        auto x = ts.make_statevar("x", bv8);
        auto inc = ts.make_term(BVAdd, x, ts.make_term(1, bv8));
        auto cond = ts.make_term(Equal, x, ts.make_term(10, bv8));
        auto zero = ts.make_term(0, bv8);
        auto fx = ts.make_term(Ite, cond, zero, inc);
        ts.assign_next(x, fx);
        ts.set_init(ts.make_term(Equal, x, zero));
        property = ts.make_term(BVUlt, x, ts.make_term(11, bv8));
    }
}
