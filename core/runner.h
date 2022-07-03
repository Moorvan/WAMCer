//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_RUNNER_H
#define WAMCER_RUNNER_H
#include <thread>
#include "frontends/btor2_encoder.h"
#include "engines/bmc.h"
#include "engines/k_induction.h"
#include "utils/config.h"

namespace wamcer {
    class Runner {
    public:
        static bool runBMC(const std::string& path, int bound = -1);
//        static bool runBMCWithKInduction(const std::string &path, int bound = -1);
    };
}


#endif //WAMCER_RUNNER_H
