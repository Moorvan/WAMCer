#include "core/runner.h"

using namespace wamcer;
using namespace smt;



int main() {
    logger.set_verbosity(1);
    auto path = "../btors/memory.btor2";
    Runner::runBMCWithKInduction(path, 150);
    return 0;
}

