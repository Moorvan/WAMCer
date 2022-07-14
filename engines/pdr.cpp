//
// Created by Morvan on 2022/7/12.
//

#include "pdr.h"


namespace wamcer {

    EasyPDR::EasyPDR(TransitionSystem &ts, Term &p)
            : transitionSystem(ts),
              solver(ts.solver()),
              property(p),
              unroller(ts.solver()) {}

    bool EasyPDR::run(int bound) {

    }
}