//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_K_INDUCTION_H
#define WAMCER_K_INDUCTION_H

#include "core/unroller.h"
#include "utils/logger.h"

using namespace smt;

namespace wamcer {
    class KInduction {
    public:
        KInduction(TransitionSystem &transitionSystem, Term &property, int &safeBound);
        bool run();

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;
        int &safeBound;
    };
}


#endif //WAMCER_K_INDUCTION_H
