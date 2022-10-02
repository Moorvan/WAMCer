//
// Created by Morvan on 2022/10/2.
//

#include "solverFactory.h"

namespace wamcer {
    void SolverFactory::setIncremental(smt::SmtSolver &solver) {
        solver->set_opt("incremental", "true");
    }

    void SolverFactory::setModelProduce(SmtSolver &solver) {
        solver->set_opt("produce-models", "true");
    }

    SmtSolver SolverFactory::boolectorSolver() {
        auto s = BoolectorSolverFactory::create(true);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::bitwuzlaSolver() {
        auto s = BitwuzlaSolverFactory::create(true);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::z3Solver() {
        auto s = Z3SolverFactory::create(true);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::cvc5Solver() {
        auto s = Cvc5SolverFactory::create(true);
        return s;
    }

    void SolverFactory::setUnsatCore(SmtSolver &solver) {
        solver->set_opt("produce-unsat-assumptions", "true");
    }
}