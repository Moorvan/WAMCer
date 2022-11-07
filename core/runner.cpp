//
// Created by Morvan on 2022/7/2.
//

#include "runner.h"
#include "solverFactory.h"
#include "engines/fbmc.h"

namespace wamcer {
    bool Runner::runBMC(const std::string path, void (*decoder)(std::string, TransitionSystem &, Term &),
                        smt::SmtSolver(solverFactory)(), int bound) {
        logger.log(defines::logBMCRunner, 0, "file: {}.", path);
        logger.log(defines::logBMCRunner, 0, "BMC running...");
        auto s = solverFactory();
        auto ts = TransitionSystem(s);
        auto p = Term();
        decoder(path, ts, p);
        auto bmc = BMC(ts, p);
        auto res = bmc.run(bound);
        if (res) {
            logger.log(defines::logBMCRunner, 0, "Result: safe in {} steps.", bound);
        } else {
            logger.log(defines::logBMCRunner, 0, "Result: unsafe.");
        }
        return res;
    }

    bool Runner::runBMCWithKInduction(const std::string path, void (*decoder)(std::string, TransitionSystem &, Term &),
                                      smt::SmtSolver (*solverFactory)(), int bound) {
        logger.log(defines::logBMCKindRunner, 0, "file: {}", path);
        logger.log(defines::logBMCKindRunner, 0, "BMC + K-Induction running...");
        auto safe = int();
        auto mux = std::mutex();
        auto cv = std::condition_variable();
        auto finish = std::condition_variable();
        auto signalExit = std::promise<void>();
        auto signalExitFuture = signalExit.get_future();
        auto isFinished = false;
        auto finishedID = std::atomic<int>{0};
        auto bmcRes = bool();
        auto kindRes = bool();
        auto wake = std::thread([&] {
            timer::wakeEvery(config::wakeKindCycle, cv, isFinished);
        });
        auto bmcRun = std::thread([&] {
            auto s = solverFactory();
            auto ts = TransitionSystem(s);
            auto p = Term();
            decoder(path, ts, p);
            auto bmc = BMC(ts, p, safe, mux, cv);
            bmcRes = bmc.run(bound);
            finishedID = 1;
            finish.notify_all();
        });
        auto kindRun = std::thread([&] {
            auto s = solverFactory();
            auto ts = TransitionSystem(s);
            auto p = Term();
            decoder(path, ts, p);
            auto kind = KInduction(ts, p, safe, mux, cv, std::move(signalExitFuture));
            kindRes = kind.run(bound);
            finishedID = 2;
            finish.notify_all();
        });
        {
            auto lck = std::unique_lock(mux);
            finish.wait(lck);
        }
        isFinished = true;
        wake.detach();
        bmcRun.detach();
        kindRun.detach();

        if (finishedID == 2) {
            logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
            return true;
        } else if (finishedID == 1) {
            if (bmcRes) {
                logger.log(defines::logBMCKindRunner, 0, "Result: safe in {} steps.", bound);
                {
                    auto lck = std::unique_lock(mux);
                    finish.wait(lck);
                }
                if (kindRes) {
                    logger.log(defines::logBMCKindRunner, 0, "Result: safe.");
                    return true;
                } else {
                    logger.log(defines::logBMCKindRunner, 0, "Result: we only know it's safe in {} steps.", bound);
                    return false;
                }
            } else {
                logger.log(defines::logBMCKindRunner, 0, "Result: unsafe.");
                signalExit.set_value();
                return false;
            }
        } else {
            logger.log(defines::logBMCKindRunner, 0, "Something wrong...");
            return false;
        }
    }



    bool Runner::runFBMCWithKInduction(std::string path, void (*decoder)(std::string, TransitionSystem &, Term &),
                                       std::function<smt::SmtSolver()> solverFactory, int bound, int termRelationLevel,
                                       int complexPredsLevel, int simFilterStep) {
        logger.log(defines::logFBMCKindRunner, 0, "file: {}", path);
        logger.log(defines::logFBMCKindRunner, 0, "FBMC + K-Induction running...");
        auto safe = int();
        auto mux = std::mutex();
        auto cv = std::condition_variable();
        auto finish = std::condition_variable();
        auto res = std::atomic<bool>();
        auto isFinished = false;

        auto bmcSlv = solverFactory();
        auto predSolver = solverFactory();

        auto preds = new AsyncTermSet();
        auto bmcExit = std::promise<void>();
        auto bmcExitFuture = bmcExit.get_future();

        auto bmcTs = TransitionSystem(bmcSlv);
        auto bmcP = Term();
        decoder(path, bmcTs, bmcP);

        // preds gen
        auto predsGen = new DirectConstructor(bmcTs, bmcP, *preds, predSolver);
        predsGen->generatePreds(termRelationLevel, complexPredsLevel);

        // bmc prove & preds filter1
        auto bmcRun = std::thread([&] {
            auto fbmc = FBMC(bmcTs, bmcP, *preds, safe, std::move(bmcExitFuture));
            auto fbmcRes = fbmc.run(bound);
            finish.notify_all();
            if (fbmcRes) {
                logger.log(defines::logFBMC, 0, "Result: safe in {} steps.", bound);
            } else {
                logger.log(defines::logFBMC, 0, "Result: unsafe.");
                res = false;
                finish.notify_all();
            }
        });
        bmcRun.detach();
        logger.log(defines::logFBMCKindRunner, 1, "Predicates: {}.", preds->size());

        // preds filter2
        if (simFilterStep > 0) {
            auto simFilterRun = std::thread([&] {
                auto simFilter = FilterWithSimulation(path, simFilterStep);
                preds->filter([&](Term t) -> bool {
                    return not simFilter.checkSat(t);
                });
            });
            simFilterRun.detach();
        }

        auto wakeup = std::thread([&] {
            timer::wakeEvery(config::wakeKindCycle, cv, isFinished);
        });

        // prove
        auto kind = std::thread([&] {
            int curCnt;
            while (true) {
                curCnt = preds->size();
                logger.log(defines::logFBMCKindRunner, 1, "run k induction with {} predicates", curCnt);
                auto signalExit = std::promise<void>();
                auto signalExitFuture = signalExit.get_future();
                auto kind0Run = std::thread([&](std::future<void> signalExitFuture) {
                    auto kind_slv = solverFactory();
                    auto ts = TransitionSystem(kind_slv);
                    auto to_ts = TermTranslator(kind_slv);
                    auto p = Term();
                    decoder(path, ts, p);
                    std::cout << preds->size() << std::endl;
                    p = preds->reduce([&](Term a, Term b) -> Term {
                        return kind_slv->make_term(And, a, to_ts.transfer_term(b));
                    }, p);
                    auto kind = KInduction(ts, p, safe, mux, cv, std::move(signalExitFuture));
                    auto kindRes = kind.run();
                    if (kindRes) {
                        logger.log(defines::logKind, 0, "Result: safe. with {} predicates: ", curCnt);
                        res = true;
                        finish.notify_all();
                    }
                }, std::move(signalExitFuture));
                while (preds->size() == curCnt && !isFinished) {
                    auto lck = std::unique_lock(mux);
                    cv.wait(lck);
                }
                signalExit.set_value();
                kind0Run.join();
                if (isFinished) {
                    return;
                }
            }
        });
        {
            // wait for finish. There is maybe a bug here. If finished before wait, it will wait forever.
            auto lck = std::unique_lock(mux);
            finish.wait(lck);
        }
        isFinished = true;
        wakeup.detach();
        kind.join();
        if (res) {
            logger.log(defines::logFBMCKindRunner, 0, "Result: safe.");
            return true;
        } else {
            logger.log(defines::logFBMCKindRunner, 0, "Result: unsafe.");
            return false;
        }
    }

    bool Runner::runPredCP(const std::string& path, const std::function<void(std::string &, TransitionSystem &)>& decode,
                           const std::function<smt::SmtSolver()>&, int bound) {

        return false;
    }
}
