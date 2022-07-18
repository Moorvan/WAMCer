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
              unroller(ts) {
        auto Bool = solver->make_sort(BOOL);
        invLabel = solver->make_symbol("inv_label", Bool);
    }

    bool EasyPDR::run(int bound) {
        auto ts = unroller.at_time(transitionSystem.trans(), 1);
        solver->assert_formula(ts);
        auto p = unroller.at_time(candidateInv, 1);
        solver->assert_formula(solver->make_term(Implies, invLabel, candidateInv));


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
        invPrim = unroller.at_time(candidateInv, 2); // candidateInv'
        auto res = solver->check_sat_assuming({notP, invLabel});
        // candidateInv /\ trans /\ ¬candidateInv'
        if (res.is_unsat()) {
            return true;
        }
        while (res.is_sat()) {
            blockModel();
            res = solver->check_sat_assuming({notP, invLabel});
        }
        return false;
    }

    void EasyPDR::blockModel() {
        auto states = TermVec();
        auto inputs = TermVec();
        getModel(states, inputs);
        logger.log(defines::logEasyPDR, 2, "new blocked model: {}", solver->make_term(And, states));
        auto m1 = getModelCore(states, inputs);
        logger.log(defines::logEasyPDR, 2, "new blocked model core: {}", m1);
        auto m = unroller.untime(m1);
        // candidateInv /\ ¬m
        candidateInv = solver->make_term(And, candidateInv, solver->make_term(Not, m));
        solver->assert_formula(solver->make_term(Implies, invLabel, solver->make_term(Not, m1)));
    }

    void EasyPDR::getModel(TermVec &stateAssigns, TermVec &inputAssign) {
        for (const auto &t: transitionSystem.statevars()) {
            auto tt = unroller.at_time(t, 1);
            auto v = solver->make_term(Equal, tt, solver->get_value(tt));
            stateAssigns.push_back(v);
        }
        for (const auto& t: transitionSystem.inputvars()) {
            auto tt = unroller.at_time(t, 1);
            auto v = solver->make_term(Equal, tt, solver->get_value(tt));
            inputAssign.push_back(v);
        }
    }

    Term EasyPDR::getModelCore(const TermVec& states, const TermVec& inputs) {
        solver->push();
        solver->assert_formula(solver->make_term(Not, invLabel));
        solver->assert_formula(invPrim);
        solver->assert_formula(solver->make_term(And, inputs));
        auto res = solver->check_sat_assuming(states);
        auto ret = TermVec();
        if (res.is_unsat()) {
            logger.log(defines::logEasyPDR, 2, "unsat model");
            auto out = UnorderedTermSet();
            solver->get_unsat_assumptions(out);
            for (const auto& t: out) {
                ret.push_back(t);
            }
        } else {
            logger.log(defines::logEasyPDR, 2, "sat model");
            for (const auto& t : states) {
                ret.push_back(t);
            }
        }
        solver->pop();
        return solver->make_term(And, ret);
    }
}