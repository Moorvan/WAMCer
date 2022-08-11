//
// Created by Morvan on 2022/7/2.
//

#include "runner.h"

namespace wamcer {
    bool Runner::runBMC(const std::string &path, int bound) {
        logger.log(defines::logBMCRunner, 0, "file: {}.", path);
        logger.log(defines::logBMCRunner, 0, "BMC running...");
        auto ts = TransitionSystem();
        auto p = BTOR2Encoder(path, ts).propvec().at(0);
        auto bmc = BMC(ts, p);
        auto res = bmc.run(bound);
        if (res) {
            logger.log(defines::logBMCRunner, 0, "Result: safe in {} steps.", bound);
        } else {
            logger.log(defines::logBMCRunner, 0, "Result: unsafe.");
        }
        return res;
    }

    bool Runner::runBMCWithKInduction(const std::string &path, int bound) {
        logger.log(defines::logBMCKindRunner, 0, "file: {}", path);
        logger.log(defines::logBMCKindRunner, 0, "BMC + K-Induction running...");
        auto safe = int();
        auto mux = std::mutex();
        auto cv = std::condition_variable();
        auto isFinished = std::condition_variable();
        auto finishedID = std::atomic<int>{0};
        auto bmcRes = bool();
        auto kindRes = bool();
        auto wake = std::thread([&] {
            timer::wakeEvery(config::wakeKindCycle, cv);
        });
        auto bmcRun = std::thread([&] {
            bmcRes = BMCWithKInductionBMCPart(path, bound, safe, mux, cv);
            finishedID = 1;
            isFinished.notify_all();
        });
        auto kindRun = std::thread([&] {
            kindRes = BMCWithKInductionKindPart(path, bound, safe, mux, cv);
            finishedID = 2;
            isFinished.notify_all();
        });
        auto lck = std::unique_lock(mux);
        isFinished.wait(lck);
        wake.detach();
        bmcRun.detach();
        kindRun.detach();

        if (finishedID == 2) {
            logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
            return true;
        } else if (finishedID == 1) {
            if (bmcRes) {
                logger.log(defines::logBMCKindRunner, 0, "Result: safe in {} steps.", bound);
                isFinished.wait(lck);
                if (kindRes) {
                    logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
                } else {
                    logger.log(defines::logBMCKindRunner, 0, "Result: we only know it's safe in {} steps.", bound);
                }
            } else {
                logger.log(defines::logBMCKindRunner, 0, "Result: unsafe.");
                return false;
            }
        } else {
            logger.log(defines::logBMCKindRunner, 0, "Something wrong...");
            return false;
        }
        return false;
    }

    bool Runner::BMCWithKInductionBMCPart(const std::string &path, int bound, int &safe, std::mutex &mux,
                                          std::condition_variable &cv) {
        auto ts = TransitionSystem();
        auto p = BTOR2Encoder(path, ts).propvec().at(0);
        auto bmc = BMC(ts, p, safe, mux, cv);
        return bmc.run(bound);
    }

    bool Runner::BMCWithKInductionKindPart(const std::string &path, int bound, int &safe, std::mutex &mux,
                                           std::condition_variable &cv) {
        auto ts = TransitionSystem();
        auto p = BTOR2Encoder(path, ts).propvec().at(0);
        auto kind = KInduction(ts, p, safe, mux, cv);
        return kind.run(bound);
    }

    bool Runner::runBMCWithKInduction(void (*TSGen)(TransitionSystem &transitionSystem, Term &property), int bound) {
        logger.log(defines::logBMCKindRunner, 0, "BMC + K-Induction running...");
        auto safe = int();
        auto mux = std::mutex();
        auto cv = std::condition_variable();
        auto isFinished = std::condition_variable();
        auto finishedID = std::atomic<int>{0};
        auto bmcRes = bool();
        auto kindRes = bool();
        auto wake = std::thread([&] {
            timer::wakeEvery(config::wakeKindCycle, cv);
        });
        auto bmcRun = std::thread([&] {
            auto ts = TransitionSystem();
            auto p = Term();
            TSGen(ts, p);
            auto bmc = BMC(ts, p, safe, mux, cv);
            bmcRes = bmc.run(bound);
            finishedID = 1;
            isFinished.notify_all();
        });
        auto kindRun = std::thread([&] {
            auto ts = TransitionSystem();
            auto p = Term();
            TSGen(ts, p);
            auto kind = KInduction(ts, p, safe, mux, cv);
            kindRes = kind.run(bound);
            finishedID = 2;
            isFinished.notify_all();
        });
        auto lck = std::unique_lock(mux);
        isFinished.wait(lck);
        wake.detach();
        bmcRun.detach();
        kindRun.detach();

        if (finishedID == 2) {
            logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
            return true;
        } else if (finishedID == 1) {
            if (bmcRes) {
                logger.log(defines::logBMCKindRunner, 0, "Result: safe in {} steps.", bound);
                isFinished.wait(lck);
                if (kindRes) {
                    logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
                } else {
                    logger.log(defines::logBMCKindRunner, 0, "Result: we only know it's safe in {} steps.", bound);
                }
            } else {
                logger.log(defines::logBMCKindRunner, 0, "Result: unsafe.");
                return false;
            }
        } else {
            logger.log(defines::logBMCKindRunner, 0, "Something wrong...");
            return false;
        }
        return false;
    }

    bool Runner::runFBMCWithKInduction(void (*TSGen)(TransitionSystem &, Term &), int bound) {

    }


}
