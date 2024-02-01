//
// Created by Morvan on 2023/9/17.
//

#ifndef WAMCER_INTERPOLANTBMC_H
#define WAMCER_INTERPOLANTBMC_H

#include "core/unroller.h"
#include "utils/logger.h"
#include "utils/defines.h"

using namespace smt;

namespace wamcer {
    class InterpolantBmc {
    public:
        InterpolantBmc(std::string path, const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder, const std::function<smt::SmtSolver()> &solverFactory);

        Result getInterpolant(int k, Term &out);

        bool bmc(const Term& from, int start, int k);

    private:
        std::string path;
        std::function<void(std::string &, TransitionSystem &, Term &)> decoder;
        std::function<smt::SmtSolver()> solverFactory;
        TransitionSystem ts;
        Term p;
        SmtSolver slv;
        Unroller* unroller;
    };
}


#endif //WAMCER_INTERPOLANTBMC_H
