//
// Created by Morvan on 2022/12/13.
//

#include "PredCP.h"

namespace wamcer {

    PredCP::PredCP(TransitionSystem &transitionSystem, smt::Term &property, int max_step) :
            slv(transitionSystem.solver()),
            unroller(transitionSystem),
            ts(transitionSystem),
            prop(property),
            max_step(max_step),
            preds(max_step + 1),
            mutexes(max_step + 1),
            cvs(max_step + 1) {
        cur_step = -1;
    }

    bool PredCP::check(int at) {
        // at 可变，或者多个，当 wait 的时候，就去做别的
        if (at >= max_step) {
            return false;
        }
        auto lock = std::unique_lock<std::mutex>(global_mux);
        auto new_ts = TransitionSystem::copy(ts);
        auto bmc = BMCChecker(new_ts);
        lock.unlock();
        while (true) {
            logger.log(defines::logPredCP, 1, "PredCP: check at {}, with pred size: {}", at, preds.size(at));
            while (preds.size(at) == 0) {
                auto lck = std::unique_lock(mutexes[at]);
                cvs[at].wait(lck);
            }
            auto p = Term();
            preds.pop(p, at);
            if (p == nullptr) {
                continue;
            }
            if (bmc.check(at, p)) {
                preds.insert({p}, at + 1);
                cvs[at + 1].notify_all();
                logger.log(defines::logPredCP, 1, "check: one term checked in at step {}", at);
            } else {
                logger.log(defines::logPredCP, 1, "check: one term is not valid at {}", at);
            }
        }
    }

    bool PredCP::prove(int at) {
        if (at > max_step) {
            return false;
        }
        while (at > cur_step) {
            auto lck = std::unique_lock(prove_wait_mux);
            prove_wait_cv.wait(lck);
        }
        auto lock = std::unique_lock(global_mux);
        auto new_ts = TransitionSystem::copy(ts);
        auto prover = InductionProver(new_ts, prop);
        lock.unlock();
        auto p = slv->make_term(true);
        while (true) {
            {
                auto lck = std::unique_lock(mutexes[at]);
                while (preds.size(at) == 0) {
                    cvs[at].wait(lck);
                }
            }
            logger.log(defines::logPredCP, 1, "PredCP: prove at {}, with pred size: {}", at, preds.size(at));
            p = preds.reduce([&](const smt::Term &t1, const smt::Term &t2) -> smt::Term {
                return slv->make_term(smt::And, t1, t2);
            }, p, at);
            if (p == nullptr) {
                continue;
            }
            if (prover.prove(at, p)) {
                logger.log(defines::logPredCP, 0, "prove: {} is proved to be {} step invariant.", p, at);
                return true;
            } else {
                logger.log(defines::logPredCP, 1, "prove: one term is not proved to be {} step invariant.", at);
            }
        }
    }

    void PredCP::propBMC() {
        auto lock = std::unique_lock<std::mutex>(global_mux);
        auto new_ts = TransitionSystem::copy(ts);
        auto bmc = BMCChecker(new_ts);
        lock.unlock();
        while (true) {
            if (bmc.check(cur_step + 1, prop)) {
                logger.log(defines::logPredCP, 1, "propBMC: prop is checked safe at {} step", cur_step + 1);
                cur_step++;
                prove_wait_cv.notify_all();
            } else {
                logger.log(defines::logPredCP, 0, "propBMC: prop is checked unsafe at {} step", cur_step + 1);
                return;
            }

        }
    }

    void PredCP::insert(const smt::TermVec &terms, int at) {
        preds.insert(terms, at);
        cvs[at].notify_all();
    }

    void PredCP::insert(AsyncTermSet &terms, int at) {
        terms.map([&](const smt::Term &t) {
            preds.insert({t}, at);
        });
        cvs[at].notify_all();
    }

}