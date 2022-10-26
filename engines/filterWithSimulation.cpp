//
// Created by Morvan on 2022/10/24.
//

#include "filterWithSimulation.h"

namespace wamcer {

    FilterWithSimulation::FilterWithSimulation(std::string path, int step)
            : slv(SolverFactory::boolectorSolver()), path(path), sim_states(UnorderedTermSet()), translator(slv) {
        auto terms = sim::randomSim(path, slv, step);
        for (const auto &term: terms) {
            sim_states.insert(term);
        }
    }

    void FilterWithSimulation::addSimState(int step) {
        auto terms = sim::randomSim(path, slv, step);
        for (const auto &term: terms) {
            sim_states.insert(term);
        }
    }

    bool FilterWithSimulation::checkSat(Term &t) {
        auto tt = translator.transfer_term(t);
        slv->push();
        slv->assert_formula(tt);
        for (const auto &sim_state: sim_states) {
            if (slv->check_sat_assuming({sim_state}).is_unsat()) {
                slv->pop();
                return false;
            }
        }
        slv->pop();
        return true;
    }
}