//
// Created by Morvan on 2022/7/2.
//

#include "runner.h"

namespace wamcer {
    bool Runner::runBMC(const std::string& path, int bound) {
        logger.log(0, "file: {}.", path);
        logger.log(0, "BMC running...");
        auto ts = TransitionSystem();
        auto p = BTOR2Encoder(path, ts).propvec().at(0);
        auto bmc = BMC(ts, p);
        auto res = bmc.run(bound);
        if (res) {
            logger.log(0, "Result: safe.");
        } else {
            logger.log(0, "Check failed.");
        }
        return res;
    }


}
