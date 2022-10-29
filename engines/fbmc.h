//
// Created by Morvan on 2022/8/2.
//

#ifndef WAMCER_FBMC_H
#define WAMCER_FBMC_H

#include "engines/wamcer.h"
#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include "async/asyncTermSet.h"
#include <mutex>
#include <condition_variable>
#include <future>

using namespace smt;

namespace wamcer {

    class FBMC {
    public:
        /// \param ts transition system
        /// \param property property to check
        /// \param predicates predicates to check
        /// \param safeStep safe step reference
        /// \param mux mutex to lock predicates set
        /// \param cv condition variable to notify that predicates set initialized
        FBMC(TransitionSystem &ts, Term &property, AsyncTermSet &predicates, int &safeStep,
             std::future<void> exitSignal = std::future<void>());

        bool run(int bound = -1);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;
        TermTranslator to_solver;

        int &safeStep;
        AsyncTermSet &preds;
        std::future<void> exitSignal;
        bool exited;

        bool step0();

        bool stepN(int n);

        void filterPredsAt(int step);

    };
}


#endif //WAMCER_FBMC_H
