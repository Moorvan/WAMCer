//
// Created by Morvan on 2022/7/1.
//

#include "bmc.h"


namespace wamcer {
    BMC::BMC(TransitionSystem &ts, Term &p) {
        transitionSystem = ts;
        property = p;
        solver = ts.solver();
    }
}