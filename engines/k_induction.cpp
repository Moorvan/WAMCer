//
// Created by Morvan on 2022/7/2.
//

#include "k_induction.h"


namespace wamcer {
    KInduction::KInduction(TransitionSystem &ts, Term &p, int &safeBound, std::mutex &mux,
                           std::condition_variable &cv, std::future<void> exit_signal)
            : transitionSystem(ts),
              solver(ts.solver()),
              property(p),
              unroller(ts),
              safeBound(int()),
              safeBoundRef(safeBound),
              muxRef(mux),
              cvRef(cv) {
        this->exit_signal = std::move(exit_signal);
    }

    KInduction::KInduction(TransitionSystem &ts, Term &p)
            : KInduction(ts, p, safeBound, mux, cv) {
        safeBoundRef = defines::allStepSafe;
    }

    bool KInduction::run(int bound) {
        if (bound == 0) {
            return false;
        }
        for (int i = 1; i != bound; i++) {
            if (exit_signal.valid() &&
                exit_signal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                return false;
            }
            while (safeBoundRef != defines::allStepSafe && i > safeBoundRef + 1) {
                auto lck = std::unique_lock(muxRef);
                cvRef.wait(lck);
            }
            if (stepN(i)) {
                logger.log(defines::logKind, 1, "Proved: property is {}-inductive-invariant.", i);
                return true;
            } else {
                logger.log(defines::logKind, 1, "{}-inductive check: Failed.", i);
            }
        }
        return false;
    }

    bool KInduction::stepN(int n) {
        auto transNSub1 = unroller.at_time(transitionSystem.trans(), n - 1);
        auto propNSub1 = unroller.at_time(property, n - 1);

        solver->assert_formula(transNSub1);
        solver->assert_formula(propNSub1);
        if (checkSimplePathLazy(n)) {
            return true;
        }
        return false;
    }

    Term KInduction::simplePathConstraint(int i, int j) {
        assert(!transitionSystem.statevars().empty());

        Term disj = solver->make_term(false);
        for (auto &v: transitionSystem.statevars()) {
            Term vi = unroller.at_time(v, i);
            Term vj = unroller.at_time(v, j);
            Term eq = solver->make_term(Equal, vi, vj);
            Term neq = solver->make_term(Not, eq);
            disj = solver->make_term(Or, disj, neq);
        }
        return disj;
    }

    bool KInduction::checkSimplePathLazy(int n) {
        auto addSimplePathConstraint = false;
        auto propN = unroller.at_time(property, n);
        auto badN = solver->make_term(Not, propN);
        do {
            auto res = solver->check_sat_assuming({badN});
            if (res.is_unsat()) {
                return true;
            }
            addSimplePathConstraint = false;
            for (int j = 0; j < n && !addSimplePathConstraint; j++) {
                for (int l = j + 1; l <= n; l++) {
                    Term constraint = simplePathConstraint(j, l);
                    if (solver->get_value(constraint) == solver->make_term(false)) {
                        logger.log(defines::logKind, 2, "add simple path constraint: state_{} and state_{}.", j, l);
                        solver->assert_formula(constraint);
                        addSimplePathConstraint = true;
                        break;
                    }
                }
            }
        } while (addSimplePathConstraint);

        return false;
    }
}