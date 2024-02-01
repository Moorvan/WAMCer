//
// Created by Morvan on 2023/9/17.
//

#include "InterpolantBmc.h"

namespace wamcer {
    InterpolantBmc::InterpolantBmc(std::string path,
                                   const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                   const std::function<smt::SmtSolver()> &solverFactory) {
        p = Term();
        slv = solverFactory();
        ts = TransitionSystem(slv);
        decoder(path, ts, p);
        unroller = new Unroller(ts);
    }

     Result InterpolantBmc::getInterpolant(int k, Term &out) {

        auto init_0 = unroller->at_time(ts.init(), 0);
        auto state_k = init_0;
        for (int i = 0; i < k; i++) {
            state_k = slv->make_term(And, state_k, unroller->at_time(ts.trans(), i));
        }

        auto prop_k = unroller->at_time(p, k);
        auto bad_k = slv->make_term(Not, prop_k);

        return slv->get_interpolant(state_k, bad_k, out);
    }

    bool InterpolantBmc::bmc(const Term& from, int start, int k) {
        slv->push();
        slv->assert_formula(from);
        for (int i = 0; i < k; i++) {
            slv->assert_formula(unroller->at_time(ts.trans(), i));
        }
        auto prop_k = unroller->at_time(p, k);
        auto bad_k = slv->make_term(Not, prop_k);
        auto res = slv->check_sat_assuming({bad_k});
        slv->pop();
        if (res.is_sat()) {
            logger.log(defines::logInterpolantBMC, 1, "Check failed at {} step.", k);
            return false;
        } else {
            logger.log(defines::logInterpolantBMC, 1, "Check safe at {} step.", k);
            return true;
        }
    }

}
