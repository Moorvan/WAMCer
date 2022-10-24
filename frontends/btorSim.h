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

#include "btorsim/btorsimstate.h"
#include "btorsim/btorsimhelpers.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "smt-switch/smt.h"
#include "utils/logger.h"
#include "utils/defines.h"
#include "core/ts.h"
#include "frontends/btor2_encoder.h"

using namespace smt;
using namespace wamcer;

namespace wamcer::sim {
    TermVec
    randomSim(std::string path, SmtSolver solver, int bound = 20, int seed = (int) time(0), std::string filepath = "");
}


#endif //WAMCER_BTORSIM_H
