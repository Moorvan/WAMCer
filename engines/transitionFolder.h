//
// Created by Morvan on 2022/11/9.
//

#ifndef WAMCER_TRANSITIONFOLDER_H
#define WAMCER_TRANSITIONFOLDER_H

#include "core/ts.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include "core/unroller.h"

namespace wamcer {
    class TransitionFolder {
    public:
        TransitionFolder(TransitionSystem &ts, const smt::SmtSolver& folderSolver);

        void getNStepTrans(int n, smt::Term &out_trans, smt::TermTranslator &translator);

        void foldToNStep(int n,  const std::function<void(int, const smt::Term &)>& add_trans);

    private:
        TransitionSystem ts;
        smt::TermVec trans;
        smt::SmtSolver slv;
        smt::TermTranslator to_slv;
        smt::UnorderedTermMap updates;
        smt::UnorderedTermSet original_inputs;
        std::vector<std::pair<smt::Term, bool>> original_constraints;

        int maxTime;

        TransitionSystem fold(const TransitionSystem &in, int x);

        TransitionSystem add(const TransitionSystem &in1, const TransitionSystem &in2, int x1, int x2);

        void newTS(TransitionSystem &out);

        smt::Term substituteInput(int start, int n, smt::Term in);

        smt::Term varAtTime(smt::Term in, int time);

        void addInputs(int start, int x);

    };
}


#endif //WAMCER_TRANSITIONFOLDER_H
