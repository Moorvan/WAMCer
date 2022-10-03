//
// Created by Morvan on 2022/10/2.
//

#ifndef WAMCER_SOLVERFACTORY_H
#define WAMCER_SOLVERFACTORY_H

#include "smt-switch/smt.h"
#include "smt-switch/boolector_factory.h"
#include "smt-switch/bitwuzla_factory.h"
#include "smt-switch/z3_factory.h"
#include "smt-switch/cvc5_factory.h"

using namespace smt;

namespace wamcer {
    class SolverFactory {
    public:
        static SmtSolver boolectorSolver(bool logging = false);
        static SmtSolver bitwuzlaSolver(bool logging = false);
        static SmtSolver z3Solver(bool logging = false);
        static SmtSolver cvc5Solver(bool logging = false);

    private:
        static void setIncremental(smt::SmtSolver &solver);
        static void setModelProduce(smt::SmtSolver &solver);
        static void setUnsatCore(smt::SmtSolver &solver);
    };
}


#endif //WAMCER_SOLVERFACTORY_H
