//
// Created by Morvan on 2022/11/7.
//

#ifndef WAMCER_INDUCTIONPROVER_H
#define WAMCER_INDUCTIONPROVER_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"

namespace wamcer {
    class InductionProver {
    public:
        InductionProver(TransitionSystem &transitionSystem, smt::Term &property);

        bool prove(int at, const smt::Term& t);

    private:
        smt::SmtSolver slv;
        TransitionSystem ts;
        smt::Term p;
        Unroller unroller;
        smt::TermTranslator* to_slv;

        int step;

        void grow(int n);

        smt::Term simplePathConstraint(int i, int j);

    };
}

#endif //WAMCER_INDUCTIONPROVER_H
