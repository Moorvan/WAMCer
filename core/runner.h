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
#include "engines/filterWithSimulation.h"
#include "config.h"
#include "utils/defines.h"
#include "utils/timer.h"
#include "async/asyncTermSet.h"
#include "engines/DirectConstructor.h"
#include "engines/InductionProver.h"

namespace wamcer {
    class Runner {
    public:
        static bool runBMC(std::string path, void(*decoder)(std::string, TransitionSystem &, Term &),
                           smt::SmtSolver(solverFactory)(),
                           int bound = -1);

        static bool runBMCWithKInduction(std::string path,
                                         void (*decoder)(std::string, TransitionSystem &, Term &),
                                         smt::SmtSolver(solverFactory)(),
                                         int bound = -1);

        static bool runFBMCWithKInduction(std::string path,
                                          const std::function<void(std::string &, TransitionSystem &, Term&)> &decoder,
                                          std::function<smt::SmtSolver()>,
                                          int bound = -1,
                                          int termRelationLevel = 0,
                                          int complexPredsLevel = 1,
                                          int simFilterStep = 0);

        static bool
        runPredCP(const std::string &path,
                  const std::function<void(std::string &, TransitionSystem &)> &decoder,
                  const std::function<smt::SmtSolver()> &,
                  int bound = -1);

    private:
        static bool checkInv(std::string path,
                             const std::function<void(std::string &, TransitionSystem &, Term&)> &decoder,
                             Term inv,
                             int bound);
    };
}


#endif //WAMCER_RUNNER_H
