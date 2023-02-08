//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_RUNNER_H
#define WAMCER_RUNNER_H

#include <thread>
#include <atomic>
#include <random>
#include "smt-switch/smt.h"
#include "frontends/btor2_encoder.h"
#include "engines/bmc.h"
#include "engines/k_induction.h"
#include "engines/filterWithSimulation.h"
#include "engines/transitionFolder.h"
#include "engines/BMCChecker.h"
#include "config.h"
#include "utils/defines.h"
#include "utils/timer.h"
#include "async/asyncTermSet.h"
#include "engines/DirectConstructor.h"
#include "engines/InductionProver.h"

namespace wamcer {
    class Runner {
    public:
        static bool runBMC(std::string path,
                           const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                           smt::SmtSolver(solverFactory)(),
                           int bound = -1);

        static bool runBMCWithKInduction(std::string path,
                                         const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                         smt::SmtSolver(solverFactory)(),
                                         int bound = -1);

        static bool runFBMCWithKInduction(std::string path,
                                          const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                          std::function<smt::SmtSolver()>,
                                          int bound = -1,
                                          int termRelationLevel = 0,
                                          int complexPredsLevel = 1,
                                          int simFilterStep = 0);

        static bool runBMCWithFolder(std::string path,
                                     const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                     const std::function<smt::SmtSolver()> &solverFactory,
                                     int bound = -1, int foldThread = 1, int checkThread = 2);

        static bool
        runPredCP(std::string path,
                  const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                  const std::function<smt::SmtSolver()> &solverFactory,
                  const std::function<void(TransitionSystem&, Term&, AsyncTermSet&, SmtSolver&)>& gen,
                  int bound = -1);

    private:
        static bool checkInv(std::string path,
                             const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                             const Term& inv,
                             int bound);
    };
}


#endif //WAMCER_RUNNER_H
