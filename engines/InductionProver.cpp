//
// Created by Morvan on 2022/11/7.
//

#include "InductionProver.h"

namespace wamcer {

    InductionProver::InductionProver(TransitionSystem &transitionSystem, smt::Term &property) :
            ts(transitionSystem),
            slv(transitionSystem.solver()),
            unroller(transitionSystem),
            p(property) {
        to_slv = new smt::TermTranslator(slv);
        step = 0;
    }

    bool InductionProver::prove(int at, const smt::Term& t) {
        if (at > step) {
            grow(at);
        }
        auto tt = slv->make_term(smt::And, to_slv->transfer_term(t), p);
        auto constraints = smt::TermVec();
        slv->push();
        for (auto i = 0; i < at; i++) {
            slv->assert_formula(unroller.at_time(tt, i));
        }
        auto notTn = slv->make_term(smt::Not, unroller.at_time(tt, at));
        slv->assert_formula(notTn);
        auto added = false;
        do {
            if (slv->check_sat().is_unsat()) {
                return true;
            }
            added = false;
            for (int j = 0; j < at && !added; j++) {
                for (int l = j + 1; l <= at; l++) {
                    smt::Term constraint = simplePathConstraint(j, l);
                    if (slv->get_value(constraint) == slv->make_term(false)) {
                        slv->assert_formula(constraint);
                        constraints.push_back(constraint);
                        added = true;
                        break;
                    }
                }
            }
        } while (added);
        slv->pop();
        for (const auto& constraint : constraints) {
            slv->assert_formula(constraint);
        }
        return false;
    }

    void InductionProver::grow(int n) {
        if (n <= step) {
            return;
        }
        while (step < n) {
            auto trans = unroller.at_time(ts.trans(), step);
            slv->assert_formula(trans);
            step++;
        }
    }

    smt::Term InductionProver::simplePathConstraint(int i, int j) {
        assert(!ts.statevars().empty());
        smt::Term disj = slv->make_term(false);
        for (auto &v: ts.statevars()) {
            smt::Term vi = unroller.at_time(v, i);
            smt::Term vj = unroller.at_time(v, j);
            smt::Term eq = slv->make_term(smt::Equal, vi, vj);
            smt::Term neq = slv->make_term(smt::Not, eq);
            disj = slv->make_term(smt::Or, disj, neq);
        }
        return disj;
    }
}