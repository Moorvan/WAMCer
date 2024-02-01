//
// Created by Morvan on 2023/9/17.
//

#include "AutoRestartBMC.h"


namespace wamcer {
    AutoRestartBMC::AutoRestartBMC(std::string path, const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder, const std::function<smt::SmtSolver()> &solverFactory)
    : path(path), decoder(decoder), solverFactory(solverFactory) {
        restart();
    }

    void AutoRestartBMC::restart() {
        p = Term();
        slv = solverFactory();
        ts = TransitionSystem(slv);
        decoder(path, ts, p);
        unroller = new Unroller(ts);
        auto init0 = unroller->at_time(ts.init(), 0);
        slv->assert_formula(init0);
        logger.log(defines::logBMC, 2, "add init into slv");
        step = 0;
        logger.log(defines::logBMC, 1, "restart bmc");
    }

    bool AutoRestartBMC::check(int k) {
        if (k > step) {
            for (int i = step; i < k; i++) {
                auto trans_i = unroller->at_time(ts.trans(), i);
                slv->assert_formula(trans_i);
                logger.log(defines::logBMC, 2, "add trans_{} into slv", i);
                step++;
            }
        }
        auto prop_k = unroller->at_time(p, k);
        auto bad_k = slv->make_term(Not, prop_k);
        auto res = slv->check_sat_assuming({bad_k});
        if (res.is_sat()) {
            logger.log(defines::logBMC, 1, "Check failed at {} step.", k);
            return false;
        } else {
            auto out = UnorderedTermSet();
            slv->get_unsat_assumptions(out);
            for (auto &term : out) {
                logger.log(defines::logBMC, 2, "unsat assumption: {}", term);
            }
            logger.log(defines::logBMC, 1, "Check safe at {} step.", k);
            return true;
        }
    }
}