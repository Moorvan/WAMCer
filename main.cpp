#include "core/runner.h"
#include "core/solverFactory.h"

using namespace wamcer;
using namespace smt;


int main() {
    logger.set_verbosity(1);
    auto path = "../btors/counter-101.btor2";
    Runner::runBMCWithKInduction(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, 1000000);
    return 0;
}

