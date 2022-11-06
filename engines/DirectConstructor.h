//
// Created by Morvan on 2022/10/29.
//

#ifndef WAMCER_DIRECTCONSTRUCTOR_H
#define WAMCER_DIRECTCONSTRUCTOR_H

#include "smt-switch/smt.h"
#include "core/ts.h"
#include "async/asyncTermSet.h"
#include "utils/defines.h"
#include "utils/logger.h"
#include "engines/wamcer.h"

namespace wamcer {
    // Direct constructor for preds from ts.
    class DirectConstructor {
    public:
        DirectConstructor(TransitionSystem &ts, Term &property, AsyncTermSet &preds, const SmtSolver predSolver);

        /// \param termRelationLevel default 0.
        /// 0: only collect equivalence relation between terms \n
        /// 1: collect equivalence, unsigned greater than, signed greater than relation between terms
        /// \param complexPredLevel construct complex predicates from base predicates as t0 /\ t1 /\ ... /\ t{k-1} -> t{k}. default 1. \n
        /// 0: k = 0 \n
        /// 1: k = 1 \n
        /// 2: k = 2 \n
        void generatePreds(int termRelationLvl = 0, int complexPredsLvl = 1);

    private:
        SmtSolver solver;
        TransitionSystem &transitionSystem;
        Term property;
        AsyncTermSet &preds;
        TermTranslator to_preds;
        UnorderedTermSet basePreds;
        int termRelationLevel;
        int complexPredsLevel;

        void collectStructuralPredicates();

        void constructTermsRelation();

        void constructComplexPreds();

        void addToBasePreds(TermVec terms);

        SortTermSetMap collectTerms();
    };
}


#endif //WAMCER_DIRECTCONSTRUCTOR_H
