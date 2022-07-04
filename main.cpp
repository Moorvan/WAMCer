#include <thread>
#include <chrono>
#include "core/runner.h"

using namespace wamcer;
using namespace smt;



int main() {
    logger.set_verbosity(1);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    Runner::runBMCWithKInduction(path, 150);
    return 0;
}

