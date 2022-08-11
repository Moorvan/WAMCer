//
// Created by Morvan on 2022/8/2.
//

#ifndef WAMCER_FBMC_H
#define WAMCER_FBMC_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"

using namespace smt;

namespace wamcer {

    class FBMC {
    public:
        FBMC(TransitionSystem &ts, Term &property, UnorderedTermSet &predicates, int& safeStep, std::mutex& mux, std::condition_variable& cv, TermTranslator& to_preds);

        bool run(int bound = -1);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;
        TermTranslator& to_preds;
        TermTranslator to_solver;

        int &safeStep;
        std::mutex& mux;
        std::condition_variable& cv;
        UnorderedTermSet &preds;

        void predicateCollect();

        bool step0();
        bool stepN(int n);

        void filterPredsAt(int step);

    };
}


#endif //WAMCER_FBMC_H
