//
// Created by Morvan on 2022/12/13.
//

#ifndef WAMCER_PREDCP_H
#define WAMCER_PREDCP_H

#include <atomic>
#include "async/asyncPreds.h"
#include "core/unroller.h"
#include "utils/logger.h"
#include "BMCChecker.h"
#include "InductionProver.h"

namespace wamcer {
    class PredCP {
    public:
        PredCP(TransitionSystem &transitionSystem, smt::Term &property, int max_step);

        bool check(int at);

        bool prove(int at);

        void propBMC();

        void insert(const smt::TermVec& terms, int at);

        void insert(AsyncTermSet &terms, int at);

    private:
        std::mutex global_mux;
        std::vector<std::mutex> mutexes;
        std::vector<std::condition_variable> cvs;
        std::mutex prove_wait_mux;
        std::condition_variable prove_wait_cv;

        smt::SmtSolver slv;
        Unroller unroller;
        TransitionSystem ts;
        smt::Term prop;

        AsyncPreds preds;
        int max_step;
        std::atomic<int> cur_step;
    };
}


#endif //WAMCER_PREDCP_H
