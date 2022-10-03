//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_RUNNER_H
#define WAMCER_RUNNER_H

#include <thread>
#include <atomic>
#include "smt-switch/smt.h"
#include "frontends/btor2_encoder.h"
#include "engines/bmc.h"
#include "engines/k_induction.h"
#include "config.h"
#include "utils/defines.h"
#include "utils/timer.h"

namespace wamcer {
    class Runner {
    public:
        static bool
        runBMC(std::string path, void(*decoder)(std::string, TransitionSystem &, Term &),
               smt::SmtSolver(solverFactory)(),
               int bound = -1);

        static bool
        runBMCWithKInduction(std::string path, void (*decoder)(std::string, TransitionSystem &, Term &),
                             smt::SmtSolver(solverFactory)(), int bound = -1);

//        static bool runBMCWithKInduction(std::string path, int bound = -1);
//
//        static bool
//        runBMCWithKInduction(void (*TSGen)(TransitionSystem &transitionSystem, Term &property), int bound = -1);
//
//        static bool
//        runFBMCWithKInduction(void (*TSGen)(TransitionSystem &transitionSystem, Term &property), int bound = -1);


//    private:
//        static bool BMCWithKInductionBMCPart(std::string path, int bound, int &safe, std::mutex &mux,
//                                             std::condition_variable &cv);
//
//        static bool BMCWithKInductionKindPart(std::string path, int bound, int &safe, std::mutex &mux,
//                                              std::condition_variable &cv);
    };
}


#endif //WAMCER_RUNNER_H
