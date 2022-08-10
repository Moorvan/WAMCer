//
// Created by Morvan on 2022/8/2.
//

#include "fbmc.h"


namespace wamcer {

    FBMC::FBMC(TransitionSystem &ts, Term &property, UnorderedTermSet &predicates, int &safeStep, std::mutex &mux,
               std::condition_variable &cv, TermTranslator &to_preds) :
            preds(predicates),
            mux(mux),
            cv(cv),
            safeStep(safeStep),
            transitionSystem(ts),
            property(property),
            solver(ts.solver()),
            unroller(ts),
            to_preds(to_preds) {}

    bool FBMC::run(int bound) {
        logger.log(defines::logFBMC, 1, "init: {}", transitionSystem.init());
        logger.log(defines::logFBMC, 2, "trans: {}", transitionSystem.trans());
        logger.log(defines::logFBMC, 1, "prop: {}", property);
        predicateCollect();

        if(!step0()) {
            logger.log(defines::logFBMC, 0, "Check failed at init step.");
            return false;
        } else {
            logger.log(defines::logFBMC, 0, "Check safe at init step");
        }
        safeStep = 0;

        if (bound == 0) {
            return true;
        }

        for (int i = 1; i != bound; i++) {
            if (!stepN(i)) {
                logger.log(defines::logFBMC, 0, "Check failed at {} step.", i);
                return false;
            } else {
                logger.log(defines::logFBMC, 0, "Check safe at {} step.", i);
                safeStep = i;
            }
        }
        logger.log(defines::logFBMC, 0, "Safe in {} steps.", bound);
        return true;
    }

    void FBMC::predicateCollect() {

        auto atomicPreds = UnorderedTermSet();
        auto bv1 = transitionSystem.make_sort(BV, 1);
        auto bool_ = transitionSystem.make_sort(BOOL);

        for (auto v : transitionSystem.statevars()) {
            if (v->get_sort() == bv1 or v->get_sort() == bool_) {
                atomicPreds.insert(v);
            }
        }

        std::function<void(Term)> dfs = [&](Term t) {
            auto hasOnlyCurSymbol = true;
            for (auto v: t) {
                if (v->get_op() != Op()) {
                    hasOnlyCurSymbol = false;
                    dfs(v);
                } else if (transitionSystem.is_next_var(v)) {
                    hasOnlyCurSymbol = false;
                }
           }
            if (hasOnlyCurSymbol) {
                if (t->get_sort() == bv1 or t->get_sort() == bool_) {
                    atomicPreds.insert(t);
                }
            }
        };

        dfs(transitionSystem.init());
        dfs(transitionSystem.trans());
        dfs(property);

        logger.log(defines::logFBMC, 2, "atomic preds: ");
        for (auto v : atomicPreds) {
            logger.log(defines::logFBMC, 2, "{}", v);
        }

        for (auto v: atomicPreds) {
            preds.insert(to_preds.transfer_term(v));
        }

        for (auto v1: atomicPreds) {
            for (auto v2: atomicPreds) {
                for (auto r : atomicPreds) {
                    auto v1AndV2 = transitionSystem.make_term(BVAnd, v1, v2);
                    auto pred = transitionSystem.make_term(Implies, v1AndV2, r);
                    preds.insert(to_preds.transfer_term(pred));
                }
            }
        }
    }

    bool FBMC::step0() {
        auto init0 = unroller.at_time(transitionSystem.init(), 0);
        logger.log(defines::logBMC, 1, "init0: {}", init0);
        solver->assert_formula(init0);
        auto prop0 = unroller.at_time(property, 0);
        auto bad0 = solver->make_term(Not, prop0);
        auto res = solver->check_sat_assuming({bad0});
        if (res.is_sat()) {
            logger.log(defines::logBMC, 1, "init0 /\\ bad0 is sat.");
            return false;
        } else {
            logger.log(defines::logBMC, 1, "init0 /\\ bad0 is unsat.");
            return true;
        }
    }

    bool FBMC::stepN(int n) {
        auto transNSub1 = unroller.at_time(transitionSystem.trans(), n - 1);
        solver->assert_formula(transNSub1);
        auto propN = unroller.at_time(property, n);
        auto badN = solver->make_term(Not, propN);
        auto res = solver->check_sat_assuming({badN});
        if (res.is_sat()) {
            logger.log(defines::logBMC, 1, "init0 /\\ trans...{} /\\ bad{} is sat.", n, n);
            return false;
        } else {
            logger.log(defines::logBMC, 1, "init0 /\\ trans...{} /\\ bad{} is unsat.", n, n);
            return true;
        }
    }

    void FBMC::FilterPredsAt(int step) {
//        solver->get_value()
    }
}