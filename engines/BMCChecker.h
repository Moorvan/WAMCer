//
// Created by Morvan on 2022/11/7.
//

#ifndef WAMCER_BMCCHECKER_H
#define WAMCER_BMCCHECKER_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include <shared_mutex>

namespace wamcer {
    class BMCChecker {
    public:
        explicit BMCChecker(TransitionSystem &transitionSystem);

        bool check(int at, const smt::Term& t);

        bool check(const smt::Term &trans, const smt::Term &p);

    private:
        smt::SmtSolver slv;
        TransitionSystem ts;
        Unroller unroller;
        smt::TermTranslator* to_slv;

        int step;

        void growTo(int n);
    };
}


#endif //WAMCER_BMCCHECKER_H
