//
// Created by Morvan on 2022/7/2.
//

#ifndef WAMCER_RUNNER_H
#define WAMCER_RUNNER_H
#include <thread>
#include "frontends/btor2_encoder.h"
#include "engines/bmc.h"
#include "engines/k_induction.h"
#include "config.h"
#include "utils/defines.h"
#include "utils/timer.h"

namespace wamcer {
    class Runner {
    public:
        static bool runBMC(const std::string& path, int bound = -1);
        static bool runBMCWithKInduction(const std::string &path, int bound = -1);

    private:
        static bool BMCWithKInductionBMCPart(const std::string &path, int bound, int& safe, std::mutex& mux, std::condition_variable& cv);
        static bool BMCWithKInductionKindPart(const std::string &path, int bound, int& safe, std::mutex& mux, std::condition_variable& cv);
    };
}


#endif //WAMCER_RUNNER_H
