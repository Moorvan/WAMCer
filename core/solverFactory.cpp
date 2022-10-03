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

    SmtSolver SolverFactory::boolectorSolver(bool logging) {
        auto s = BoolectorSolverFactory::create(logging);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::bitwuzlaSolver(bool logging) {
        auto s = BitwuzlaSolverFactory::create(logging);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::z3Solver(bool logging) {
        auto s = Z3SolverFactory::create(logging);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    SmtSolver SolverFactory::cvc5Solver(bool logging) {
        auto s = Cvc5SolverFactory::create(logging);
        setIncremental(s);
        setModelProduce(s);
        return s;
    }

    void SolverFactory::setUnsatCore(SmtSolver &solver) {
        solver->set_opt("produce-unsat-assumptions", "true");
    }
}