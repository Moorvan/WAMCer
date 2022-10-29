//
// Created by Morvan on 2022/8/2.
//

#ifndef WAMCER_FBMC_H
#define WAMCER_FBMC_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include "async/asyncTermSet.h"
#include <mutex>
#include <condition_variable>
#include <future>

using namespace smt;

namespace wamcer {

    using SortTermSetMap = std::unordered_map<Sort, UnorderedTermSet>;

    class FBMC {
    public:
        /// \param ts transition system
        /// \param property property to check
        /// \param predicates predicates to check
        /// \param safeStep safe step reference
        /// \param mux mutex to lock predicates set
        /// \param cv condition variable to notify that predicates set initialized
        /// \param to_preds term translator to solver saving predicates
        /// \param termRelationLevel default 0.
        /// 0: only collect equivalence relation between terms \n
        /// 1: collect equivalence, unsigned greater than, signed greater than relation between terms
        /// \param complexPredLevel construct complex predicates from base predicates as t0 /\ t1 /\ ... /\ t{k-1} -> t{k}. default 1. \n
        /// 0: k = 0 \n
        /// 1: k = 1 \n
        /// 2: k = 2 \n
        FBMC(TransitionSystem &ts, Term &property, AsyncTermSet &predicates, int &safeStep, std::mutex &mux,
             std::condition_variable &cv, TermTranslator &to_preds, int termRelationLevel = 0, int complexPredsLevel = 1, std::future<void> exitSignal = std::future<void>());

        bool run(int bound = -1);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;
        TermTranslator &to_preds;
        TermTranslator to_solver;

        int &safeStep;
        std::mutex &mux;
        std::condition_variable &cv;
        UnorderedTermSet basePreds;
        AsyncTermSet &preds;
        std::future<void> exitSignal;
        bool exited;

        int termRelationLevel;
        int complexPredsLevel;

        void collectStructuralPredicates();

        void constructTermsRelation();

        void constructComplexPreds();

        SortTermSetMap collectTerms();

        void addToBasePreds(TermVec terms);

        bool step0();

        bool stepN(int n);

        void filterPredsAt(int step);

    };
}


#endif //WAMCER_FBMC_H
