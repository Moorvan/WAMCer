//
// Created by Morvan on 2022/10/8.
//

#ifndef WAMCER_BTORSIM_H
#define WAMCER_BTORSIM_H

extern "C" {
#include "btor2parser/btor2parser.h"
#include "btorsim/btorsimbv.h"
#include "btorsim/btorsimrng.h"
#include "util/btor2mem.h"
#include "util/btor2stack.h"
}

#include <string>
#include "smt-switch/smt.h"
#include "utils/logger.h"
#include "utils/defines.h"

using namespace smt;

namespace wamcer::sim {
    Term randomSim(std::string path, SmtSolver solver, int bound = 20, int seed = 0);
}


#endif //WAMCER_BTORSIM_H
