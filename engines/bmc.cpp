//
// Created by Morvan on 2022/7/1.
//

#include "bmc.h"


namespace wamcer {
    BMC::BMC(TransitionSystem &ts, Term &p, int &safeStep, std::mutex &mux, std::condition_variable &cv)
            : transitionSystem(ts),
              property(p),
              solver(ts.solver()),
              unroller(ts),
              safeStep(-1),
              safeStepRef(safeStep),
              muxRef(mux),
              cvRef(cv) {
        safeStepRef = defines::noStepSafe;
    }

    BMC::BMC(TransitionSystem &ts, Term &p)
            : BMC(ts, p, safeStep, mux, cv) {}

    bool BMC::run(int bound) {
        logger.log(defines::logBMC, 1, "init: {}", transitionSystem.init());
        logger.log(defines::logBMC, 3, "trans: {}", transitionSystem.trans());
        logger.log(defines::logBMC, 1, "prop: {}", property);
        if (!step0()) {
            logger.log(defines::logBMC, 1, "Check failed at init step.");
            return false;
        } else {
            logger.log(defines::logBMC, 1, "Check safe at init step.");
        }
        safeStepRef = 0;

        if (bound == 0) {
            return true;
        }

        for (int i = 1; i != bound; i++) {
            if (!stepN(i)) {
                logger.log(defines::logBMC, 1, "Check failed at {} step.", i);
                return false;
            } else {
                logger.log(defines::logBMC, 1, "Check safe at {} step.", i);
                safeStepRef = i;
            }
        }
        logger.log(defines::logBMC, 1, "Safe in {} steps.", bound);
        return true;
    }

    bool BMC::step0() {
        auto init0 = unroller.at_time(transitionSystem.init(), 0);
        logger.log(defines::logBMC, 2, "init0: {}", init0);
        solver->assert_formula(init0);
        auto prop0 = unroller.at_time(property, 0);
        auto bad0 = solver->make_term(Not, prop0);
        auto res = solver->check_sat_assuming({bad0});
        if (res.is_sat()) {
            logger.log(defines::logBMC, 2, "init0 /\\ bad0 is sat.");
            return false;
        } else {
            logger.log(defines::logBMC, 2, "init0 /\\ bad0 is unsat.");
            return true;
        }
    }

    bool BMC::stepN(int n) {
        auto transNSub1 = unroller.at_time(transitionSystem.trans(), n - 1);
        solver->assert_formula(transNSub1);
        auto propN = unroller.at_time(property, n);
        auto badN = solver->make_term(Not, propN);
        auto res = solver->check_sat_assuming({badN});
        if (res.is_sat()) {
            logger.log(defines::logBMC, 2, "init0 /\\ trans...{} /\\ bad{} is sat.", n, n);
            return false;
        } else {
            logger.log(defines::logBMC, 2, "init0 /\\ trans...{} /\\ bad{} is unsat.", n, n);
            return true;
        }
    }


}