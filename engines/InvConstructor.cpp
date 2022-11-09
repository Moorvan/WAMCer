//
// Created by Morvan on 2022/11/9.
//

#include "InvConstructor.h"

namespace wamcer {

    InvConstructor::InvConstructor(TransitionSystem &ts, smt::Term &property, AsyncTermSet &invs,
                                   const smt::SmtSolver &predSolver)
            : ts(ts), p(property),
              invs(invs), solver(ts.solver()),
              to_preds(predSolver) {}

    void InvConstructor::generateInvs() {
        auto update = ts.state_updates();
        logger.log(defines::logInvConstructor, 0, "p: {}", p);
        for (auto v : update) {
            logger.log(defines::logInvConstructor, 0, "key: {}", v.first);
            logger.log(defines::logInvConstructor, 0, "v: {}", v.second);
        }
    }

}