//
// Created by Morvan on 2022/7/12.
//

#ifndef WAMCER_PDR_H
#define WAMCER_PDR_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"

using namespace smt;

namespace wamcer {
    class EasyPDR {
    public:
        EasyPDR(TransitionSystem &transitionSystem, Term &property);

        bool run(int bound = -1);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;
    };
}


#endif //WAMCER_PDR_H