//
// Created by Morvan on 2022/7/1.
//

#ifndef WAMCER_BMC_H
#define WAMCER_BMC_H

#include "core/unroller.h"
#include "utils/logger.h"

using namespace smt;


namespace wamcer {
    class BMC {
    public:
        BMC(TransitionSystem &transitionSystem, Term &property, int& safeStep, std::mutex& mux, std::condition_variable& cv);
        BMC(TransitionSystem &transitionSystem, Term &property);

        bool run(int bound);

    private:
        SmtSolver solver;
        TransitionSystem transitionSystem;
        Term property;
        Unroller unroller;

        int safeStep;
        int& safeStepRef;
        std::mutex mux;
        std::mutex& muxRef;
        std::condition_variable cv;
        std::condition_variable& cvRef;

        bool step0();
        bool stepN(int n);
    };
}


#endif //WAMCER_BMC_H
