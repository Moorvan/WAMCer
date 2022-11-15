//
// Created by Morvan on 2022/11/7.
//

#include "BMCChecker.h"

namespace wamcer {
    BMCChecker::BMCChecker(TransitionSystem &transitionSystem) :
            ts(transitionSystem), slv(transitionSystem.solver()), unroller(transitionSystem) {
        to_slv = new smt::TermTranslator(slv);
        step = 0;
        auto init0 = unroller.at_time(ts.init(), 0);
        slv->assert_formula(init0);
    }

    void BMCChecker::growTo(int n) {
        if (n <= step) {
            return;
        }
        while (step < n) {
            auto trans = unroller.at_time(ts.trans(), step);
            slv->assert_formula(trans);
            step++;
        }
    }

    bool BMCChecker::check(int at, const smt::Term& t) {
        if (at > step) {
            growTo(at);
        }
        auto tn = unroller.at_time(to_slv->transfer_term(t), at);
        auto notTn = slv->make_term(smt::Not, tn);
        auto res = slv->check_sat_assuming({notTn});
        return res.is_unsat();
    }

    bool BMCChecker::check(const smt::Term &trans, const smt::Term &p) {
        auto t_s = unroller.at_time(to_slv->transfer_term(trans), 0);
        auto p_s = unroller.at_time(to_slv->transfer_term(p), 1);
        auto notP = slv->make_term(smt::Not, p_s);
        slv->push();
        slv->assert_formula(t_s);
        slv->assert_formula(notP);
        auto res = slv->check_sat();
        slv->pop();
        return res.is_unsat();

    }

}