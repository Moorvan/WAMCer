//
// Created by Morvan on 2022/7/1.
//

#ifndef WAMCER_BMC_H
#define WAMCER_BMC_H

#include "smt-switch/bitwuzla_factory.h"
#include "smt-switch/smt.h"
#include "core/ts.h"
#include "core/unroller.h"
#include "utils/logger.h"

using namespace smt;


namespace wamcer {
    class BMC {
    public:
        BMC(TransitionSystem &transitionSystem, Term &property);
        bool run(int bound);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;

        bool step0();
        bool stepN(int n);
    };
}


#endif //WAMCER_BMC_H
