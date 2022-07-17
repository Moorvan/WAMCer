//
// Created by Morvan on 2022/7/12.
//

#include "pdr.h"


namespace wamcer {

    EasyPDR::EasyPDR(TransitionSystem &ts, Term &p)
            : transitionSystem(ts),
              solver(ts.solver()),
              property(p),
              candidateInv(p),
              unroller(ts) {}

    bool EasyPDR::run(int bound) {
        auto ts = unroller.at_time(transitionSystem.trans(), 1);
        solver->assert_formula(ts);
        auto p = unroller.at_time(candidateInv, 1);
        solver->assert_formula(p);


        for (int i = 0; i != bound; i++) {
            logger.log(defines::logEasyPDR, 1, "Run in {} step", i);
            if (stepN(i)) {
                logger.log(defines::logEasyPDR, 1, "get {}-inductive invariant: {}", 1, candidateInv);
                return true;
            }
        }
        return false;
    }

    bool EasyPDR::stepN(int step) {
        auto notP = unroller.at_time(solver->make_term(Not, candidateInv), 2);
        auto res = solver->check_sat_assuming({notP});
        if (res.is_unsat()) {
            return true;
        }
        while (res.is_sat()) {
            blockModel();
            res = solver->check_sat_assuming({notP});
        }
        return false;
    }

    void EasyPDR::blockModel() {
        auto m = getModel();
        logger.log(defines::logEasyPDR, 2, "new blocked model: {}", m);
        auto m1 = unroller.at_time(m, 1);
        // candidateInv /\ ¬m
        candidateInv = solver->make_term(And, candidateInv, solver->make_term(Not, m));
        solver->assert_formula(solver->make_term(Not, m1));
    }

    Term EasyPDR::getModel() {
        auto model = solver->make_term(true);
        for (auto t: transitionSystem.statevars()) {
            auto tt = unroller.at_time(t, 1);
            auto v = solver->make_term(Equal, t, solver->get_value(tt));
            model = solver->make_term(And, model, v);
        }
        return model;
    }
}