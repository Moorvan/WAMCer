//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_K_INDUCTION_H
#define WAMCER_K_INDUCTION_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include <mutex>
#include <condition_variable>
#include <future>


using namespace smt;

namespace wamcer {
    class KInduction {
    public:
        KInduction(TransitionSystem &transitionSystem, Term &property, std::atomic<int> &safeBound, std::mutex &mux, std::condition_variable& cv, std::future<void> exit_signal = std::future<void>());
        KInduction(TransitionSystem &transitionSystem, Term &property);
        bool run(int bound = -1);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;

        std::atomic<int> safeBound;
        std::atomic<int> &safeBoundRef;
        std::mutex mux;
        std::mutex& muxRef;
        std::condition_variable cv;
        std::condition_variable& cvRef;
        std::future<void> exit_signal;

        // prove if p is n-inductive invariant
        bool stepN(int n);

        // get formula ¬(state_i = state_j)
        Term simplePathConstraint(int i, int j);

        // check if p is invariant and add simple path constraint
        bool checkSimplePathLazy(int n);

    };
}


#endif //WAMCER_K_INDUCTION_H
