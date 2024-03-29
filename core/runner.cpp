//

#include "runner.h"
#include "solverFactory.h"
#include "engines/fbmc.h"
#include "engines/PredCP.h"

namespace wamcer {
    bool Runner::runBMC(std::string path,
                        const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
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

    bool Runner::runBMCWithKInduction(std::string path,
                                      const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                      smt::SmtSolver (*solverFactory)(), int bound) {
        logger.log(defines::logBMCKindRunner, 0, "file: {}", path);
        logger.log(defines::logBMCKindRunner, 0, "BMC + K-Induction running...");
        auto safe = std::atomic<int>();
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


    bool Runner::runFBMCWithKInduction(std::string path,
                                       const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                       std::function<smt::SmtSolver()> solverFactory,
                                       int bound,
                                       int termRelationLevel,
                                       int complexPredsLevel,
                                       int simFilterStep) {
        logger.log(defines::logFBMCKindRunner, 0, "file: {}", path);
        logger.log(defines::logFBMCKindRunner, 0, "FBMC + K-Induction running...");
        auto safe = std::atomic<int>(0);
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
        logger.log(defines::logFBMCKindRunner, 1, "Predicates: {}.", preds->size());

        bmcRun.detach();
//         preds filter2
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
                    logger.log(defines::logKind, 0, "inv: {}", p);
                    std::cout << preds->size() << std::endl;
                    if (safe >= 0) {
                        p = preds->reduce([&](Term a, Term b) -> Term {
                            return kind_slv->make_term(And, a, to_ts.transfer_term(b));
                        }, p);
                    }
                    auto kind = KInduction(ts, p, safe, mux, cv, std::move(signalExitFuture));
                    auto kindRes = kind.run();
                    if (kindRes && preds->size() == curCnt) {
                        logger.log(defines::logKind, 0, "Result: safe. with {} predicates: ", curCnt);
//                        if (checkInv(path, decoder, p, safe + 1)) {
//                            logger.log(defines::logKind, 0, "Induction check: True.");
//                            logger.log(defines::logKind, 0, "inv: {}", p);
//                        } else {
//                            logger.log(defines::logKind, 0, "Something wrong.");
//                            exit(1);
//                        }
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
        kind.detach();
        wakeup.detach();
        if (res) {
            logger.log(defines::logFBMCKindRunner, 0, "Result: safe.");
            return true;
        } else {
            logger.log(defines::logFBMCKindRunner, 0, "Result: unsafe.");
            return false;
        }
    }

    bool
    Runner::runPredCP(std::string path,
                      const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                      const std::function<smt::SmtSolver()> &solverFactory,
                      const std::function<void(TransitionSystem &, Term &, AsyncTermSet &, SmtSolver &)> &gen,
                      int bound) {
        logger.log(defines::logPredCP, 0, "file: {}", path);
        auto s = solverFactory();
        auto ts = TransitionSystem(s);
        auto p = Term();
        decoder(path, ts, p);
        auto predCP = PredCP(ts, p, bound);
        auto threads = std::vector<std::thread>(); // 2 * bound + 1 threads

        auto preds = AsyncTermSet();
        // pred generation: preds in s
//        auto new_ts = TransitionSystem::copy(ts);
//        auto new_s = new_ts.get_solver();
//        auto to_s = TermTranslator(new_s);
//        auto pp = to_s.transfer_term(p);
//        threads.emplace_back([&] {
//            gen(new_ts, p, preds, s);
//            logger.log(defines::logPredCP, 1, "Predicates size: {}.", preds.size());
//            predCP.insert(preds, 0);
//        });

        // finished
        auto result = std::atomic(true);
        auto finished_mux = std::mutex();
        auto finished = std::condition_variable();

        auto new_ts1 = TransitionSystem::copy(ts);
        auto t1 = std::thread([&] {
            predCP.propBMC(new_ts1);
            result = false;
            finished.notify_all();
        });
        threads.push_back(std::move(t1));

        // pred in pools[k] means that safe in k steps
        // check 0 - bound-1 pools
        for (int i = 0; i < bound; i++) {
            auto new_ts2 = TransitionSystem::copy(ts);
            threads.emplace_back([&](int i, TransitionSystem nts) {
                predCP.check(i, nts);
            }, i, new_ts2);
        }
        // prove 1 - bound pools
        for (int i = 1; i <= bound; ++i) {
            auto new_ts3 = TransitionSystem::copy(ts);
            threads.emplace_back([&](int i, TransitionSystem nts) {
                predCP.prove(i, nts);
                finished.notify_all();
            }, i, new_ts3);
        }

        for (auto &t: threads) {
            t.detach();
        }
        {
            auto lck = std::unique_lock(finished_mux);
            finished.wait(lck);
        }

        return result;
    }

    bool Runner::checkInv(std::string path,
                          const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                          const Term &inv,
                          int bound) {
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        decoder(path, ts, p);
        auto prover = InductionProver(ts, p);
        return prover.prove(bound, inv);
    }

    bool Runner::runBMCWithFolder(std::string path,
                                  const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                                  const std::function<smt::SmtSolver()> &solverFactory,
                                  int bound,
                                  int foldThread,
                                  int checkThread) {
        const int empty = -1;
        const int success = -2;

        auto result = std::pair<bool, int>{true, bound + 1};
        auto result_mux = std::shared_mutex();

        auto cond_mux = std::mutex();
        auto cond = std::condition_variable();

        auto trans_mux = std::shared_mutex();
        auto trans = std::unordered_map<int, Term>();
        auto trans_slv = SolverFactory::boolectorSolver();
//        auto left_steps = std::set<int, std::greater<>>();
        auto left_steps = std::unordered_set<int>();
        auto left_steps_mux = std::shared_mutex();

        auto translator = TermTranslator(trans_slv);
        for (int i = 1; i <= bound; i++) {
            left_steps.insert(i);
        }
        auto cnt = 0;
        auto miss = 0;
        auto add_trans = [&](int i, const Term &t) {
            auto lck = std::unique_lock(trans_mux);
            auto lck2 = std::unique_lock(left_steps_mux);
            if (left_steps.find(i) != left_steps.end() && trans.find(i) == trans.end()) {
                auto trans_t = translator.transfer_term(t);
                cnt++;
                trans.insert({i, trans_t});
                left_steps.erase(i);
                cond.notify_all();
            } else {
                miss++;
            }
        };
        auto get_one_trans = [&](const SmtSolver &slv) -> std::pair<int, Term> {
            auto lck = std::unique_lock(trans_mux);
            auto lck2 = std::shared_lock(left_steps_mux);
            if (left_steps.empty() && trans.empty()) {
                return {success, nullptr};
            }
            if (trans.empty()) {
                return {empty, nullptr};
            }
            auto it = *trans.begin();
            trans.erase(it.first);
            auto translator = TermTranslator(slv);
            it.second = translator.transfer_term(it.second);
            logger.log(defines::logBMCWithFolderRunner, 1, "!!left trans size: {}", trans.size());
            return it;
        };
        auto get_one_fold_step = [&]() -> int {
            auto lck = std::shared_lock(left_steps_mux);
            if (left_steps.empty()) {
                return success;
            }
            logger.log(defines::logBMCWithFolderRunner, 1, "left steps size: {}", left_steps.size());
            int it;
            std::sample(left_steps.begin(), left_steps.end(), &it, 1, std::mt19937{std::random_device{}()});
            // get largest in left_steps
//            it = *left_steps.begin();
            return it;
        };

        auto threads = std::vector<std::thread>();

        for (auto i = 0; i < foldThread; i++) {
            auto t = std::thread([&] {
                auto slv = solverFactory();
                auto ts = TransitionSystem(slv);
                auto p = Term();
                decoder(path, ts, p);
                auto folder = TransitionFolder(ts, slv);
                while (true) {
                    auto step = get_one_fold_step();
                    if (step == success) {
                        return;
                    }
                    logger.log(defines::logBMCWithFolderRunner, 1, "folding to step {}", step);
                    folder.foldToNStep2(step, add_trans);
                }
            });
            threads.push_back(std::move(t));
        }

        for (auto i = 0; i < checkThread; i++) {
            auto t = std::thread([&] {
                auto slv = solverFactory();
                auto to_slv = TermTranslator(slv);
                auto ts = TransitionSystem(slv);
                auto p = Term();
                decoder(path, ts, p);
                auto not_p = slv->make_term(Not, p);

                auto unroller = Unroller(ts);
                slv->assert_formula(unroller.at_time(ts.init(), 0));

                auto check = [&](int t, const Term &trans) -> bool {
                    slv->push();
                    slv->assert_formula(trans);
                    slv->assert_formula(unroller.at_time(not_p, t));
                    auto res = slv->check_sat();
                    slv->pop();
                    return res.is_unsat();
                };
                while (true) {
//                    auto checker = BMCChecker(ts);
                    {
                        auto lck = std::shared_lock(result_mux);
                        if (!result.first) {
                            return;
                        }
                    }
                    auto t = get_one_trans(slv);
                    if (t.first == success) {
                        return;
                    }
                    if (t.first == empty) {
                        auto lck = std::unique_lock(cond_mux);
                        cond.wait(lck);
                        continue;
                    }
                    logger.log(defines::logBMCWithFolderRunner, 1, "checking step {}", t.first);
//                    if (checker.check(t.second, p, t.first)) {
                    if (check(t.first, t.second)) {
                        logger.log(defines::logBMCWithFolderRunner, 1, "safe at {} step", t.first);
                    } else {
                        logger.log(defines::logBMCWithFolderRunner, 1, "unsafe at {} step", t.first);
                        {
                            auto lck = std::unique_lock(result_mux);
                            result.first = false;
                            if (result.second > t.first) {
                                result.second = t.first;
                            }
                        }
                        return;
                    }
                }
            });
            threads.push_back(std::move(t));
        }

        // check init
        {
            auto slv = solverFactory();
            auto ts = TransitionSystem(slv);
            auto p = Term();
            decoder(path, ts, p);
            auto checker = BMCChecker(ts);
            if (checker.check(0, p)) {
                logger.log(defines::logBMCWithFolderRunner, 1, "safe at 0 step");
            } else {
                logger.log(defines::logBMCWithFolderRunner, 1, "unsafe at 0 step");
                {
                    auto lck = std::unique_lock(result_mux);
                    result.first = false;
                    result.second = 0;
                }
            }
        }

        for (auto &t: threads) {
            t.join();
        }
        logger.log(defines::logBMCWithFolderRunner, 2, "cnt: {}, miss: {}", cnt, miss);
        if (result.first) {
            logger.log(defines::logBMCWithFolderRunner, 0, "Result: safe.");
            return true;
        } else {
            logger.log(defines::logBMCWithFolderRunner, 0, "Result: unsafe at {} step", result.second);
            return false;
        }
    }

    bool
    Runner::runBMCs(std::string path, const std::function<void(std::string &, TransitionSystem &, Term &)> &decoder,
                    const std::function<smt::SmtSolver()> &solverFactory,
                    int bound, int threadCnt) {
        const int failed = -1;
        const int success = -2;
        const int unknown = -3;

        std::atomic<int> result = unknown;

        auto left_steps = std::list<int>();
        auto left_steps_mux = std::shared_mutex();

        for (int i = 0; i <= bound; i++) {
            left_steps.push_back(i);
        }

        auto get_one_step = [&]() -> int {
            auto lck = std::unique_lock(left_steps_mux);
            if (left_steps.empty()) {
                return success;
            }
            logger.log(defines::logBMCsRunner, 2, "left steps size: {}", left_steps.size());
            auto it = left_steps.back();
            left_steps.pop_back();
            return it;
        };

        auto threads = std::vector<std::thread>();

        for (auto i = 0; i < threadCnt; i++) {
            threads.emplace_back([&] {
                auto slv = solverFactory();
                auto ts = TransitionSystem(slv);
                auto p = Term();
                decoder(path, ts, p);
                auto bmc = BMCChecker(ts);
                while (true) {
                    auto task_step = get_one_step();
                    if (task_step == success) {
                        break;
                    }
                    if (bmc.check(task_step, p)) {
                        logger.log(defines::logBMCsRunner, 1, "safe at {} step", task_step);
                    } else {
                        logger.log(defines::logBMCsRunner, 0, "unsafe at {} step", task_step);
                        result = failed;
                    }
                }
            });
        }

        for (auto &t: threads) {
            t.join();
        }
        if (result == failed) {
            return false;
        } else {
            return true;
        }
    }
}
