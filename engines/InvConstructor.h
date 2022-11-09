//
// Created by Morvan on 2022/11/9.
//

#ifndef WAMCER_INVCONSTRUCTOR_H
#define WAMCER_INVCONSTRUCTOR_H

#include "async/asyncTermSet.h"
#include "core/ts.h"
#include "utils/logger.h"
#include "utils/defines.h"


namespace wamcer {
    class InvConstructor {
    public:
        InvConstructor(TransitionSystem &ts, smt::Term &property, AsyncTermSet &invs, const smt::SmtSolver& predSolver);

        void generateInvs();

    private:
        smt::SmtSolver solver;
        TransitionSystem &ts;
        smt::Term p;
        AsyncTermSet &invs;
        smt::TermTranslator to_preds;
    };
}


#endif //WAMCER_INVCONSTRUCTOR_H
