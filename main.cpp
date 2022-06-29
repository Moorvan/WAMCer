#include <iostream>
#include "smt-switch/boolector_factory.h"

int main() {
    auto s = smt::BoolectorSolverFactory::create(false);
    return 0;
}

