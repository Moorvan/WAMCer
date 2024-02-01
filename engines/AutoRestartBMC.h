//
// Created by Morvan on 2023/9/17.
//

#ifndef WAMCER_AUTORESTARTBMC_H
#define WAMCER_AUTORESTARTBMC_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"

using namespace smt;

namespace wamcer {
    class AutoRestartBMC {
    public:
        AutoRestartBMC(std::string path, const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder, const std::function<smt::SmtSolver()> &solverFactory);

        void restart();

        bool check(int k);

    private:
        std::string path;
        std::function<void(std::string &, TransitionSystem &, Term &)> decoder;
        std::function<smt::SmtSolver()> solverFactory;
        TransitionSystem ts;
        Term p;
        SmtSolver slv;
        int step;
        Unroller* unroller;
    };
}


#endif //WAMCER_AUTORESTARTBMC_H
