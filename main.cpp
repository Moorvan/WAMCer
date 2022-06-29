#include <iostream>
#include "smt-switch/bitwuzla_factory.h"

int main() {
    smt::SmtSolver s = smt::BitwuzlaSolverFactory::create(false);
    return 0;
}

