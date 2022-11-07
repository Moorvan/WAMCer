//
// Created by Morvan on 2022/8/2.
//

#include "fbmc.h"


namespace wamcer {

    FBMC::FBMC(TransitionSystem &ts, Term &property, AsyncTermSet &predicates, int &safeStep,
               std::future<void> exitSignal) :
            preds(predicates),
            safeStep(safeStep),
            transitionSystem(ts),
            property(property),
            solver(ts.solver()),
            unroller(ts),
            to_solver(solver),
            exitSignal(std::move(exitSignal)),
            exited(false) {}

    bool FBMC::run(int bound) {
        logger.log(defines::logFBMC, 1, "init: {}", transitionSystem.init());
        logger.log(defines::logFBMC, 2, "trans: {}", transitionSystem.trans());
        logger.log(defines::logFBMC, 1, "prop: {}", property);

        logger.log(defines::logFBMC, 2, "The initialization has {} preds.", preds.size());

        if (!step0()) {
            logger.log(defines::logFBMC, 0, "Check failed at init step.");
            return false;
        } else {
            logger.log(defines::logFBMC, 0, "Check safe at init step");
        }
        safeStep = 0;

        if (exited) {
            logger.log(defines::logFBMC, 0, "Safe at init step");
            return true;
        }

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
            if (exited) {
                logger.log(defines::logFBMC, 0, "Safe in {} steps.", i);
                return true;
            }
        }
        logger.log(defines::logFBMC, 0, "Safe in {} steps.", bound);
        return true;
    }


    bool FBMC::step0() {
        auto init0 = unroller.at_time(transitionSystem.init(), 0);
        solver->assert_formula(init0);
        auto prop0 = unroller.at_time(property, 0);
        auto bad0 = solver->make_term(Not, prop0);
        auto res = solver->check_sat_assuming({bad0});
        if (res.is_sat()) {
            logger.log(defines::logFBMC, 2, "init0 /\\ bad0 is sat.");
            return false;
        } else {
            logger.log(defines::logFBMC, 2, "init0 /\\ bad0 is unsat.");
            if (exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                exited = true;
                return true;
            }
            filterPredsAt(0);
            if (exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                exited = true;
                return true;
            }
            return true;
        }
    }

    bool FBMC::stepN(int n) {
        if (exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            exited = true;
            return true;
        }
        auto transNSub1 = unroller.at_time(transitionSystem.trans(), n - 1);
        solver->assert_formula(transNSub1);
        auto propN = unroller.at_time(property, n);
        auto badN = solver->make_term(Not, propN);
        auto res = solver->check_sat_assuming({badN});
        if (res.is_sat()) {
            logger.log(defines::logFBMC, 2, "init0 /\\ trans...{} /\\ bad{} is sat.", n, n);
            return false;
        } else {
            logger.log(defines::logFBMC, 2, "init0 /\\ trans...{} /\\ bad{} is unsat.", n, n);
            if (exitSignal.valid() && exitSignal.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                exited = true;
                return true;
            }
            filterPredsAt(n);
            return true;
        }
    }

    void FBMC::filterPredsAt(int step) {
        preds.filter([&](Term t) -> bool {
            auto slv_t = to_solver.transfer_term(t);
            auto t_at_step = unroller.at_time(slv_t, step);
            auto not_t = solver->make_term(Not, t_at_step);
            return solver->check_sat_assuming({not_t}).is_sat();
        });
    }

}