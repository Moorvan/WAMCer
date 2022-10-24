//
// Created by Morvan on 2022/10/24.
//

#ifndef WAMCER_FILTERWITHSIMULATION_H
#define WAMCER_FILTERWITHSIMULATION_H

#include "smt-switch/smt.h"
#include "utils/defines.h"
#include "core/solverFactory.h"
#include "frontends/btorSim.h"

namespace wamcer {
    class FilterWithSimulation {
    public:
        FilterWithSimulation(std::string path, int step = 20);

        void addSimState(int step = 20);

        bool checkSat(Term &t);

    private:
        SmtSolver slv;
        TermTranslator translator;
        std::string path;
        UnorderedTermSet sim_states;
    };
}


#endif //WAMCER_FILTERWITHSIMULATION_H
