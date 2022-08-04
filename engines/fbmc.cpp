//
// Created by Morvan on 2022/8/2.
//

#include "fbmc.h"


namespace wamcer {

    FBMC::FBMC(TransitionSystem &ts, Term &property, UnorderedTermSet &predicates, int &safeStep, std::mutex &mux,
               std::condition_variable &cv, TermTranslator &to_preds) :
            preds(predicates),
            mux(mux),
            cv(cv),
            safeStep(safeStep),
            transitionSystem(ts),
            property(property),
            solver(ts.solver()),
            unroller(ts),
            to_preds(to_preds) {}

    bool FBMC::run(int bound) {
        logger.log(defines::logFBMC, 1, "init: {}", transitionSystem.init());
        logger.log(defines::logFBMC, 1, "trans: {}", transitionSystem.trans());
        logger.log(defines::logFBMC, 1, "prop: {}", property);
        predicateCollect();
        return true;
    }

    void FBMC::predicateCollect() {
        auto p = property;
        logger.log(defines::logFBMC, 1, "p = {}", p);

        auto preds = UnorderedTermSet();

        std::function<void(Term)> dfs = [&](Term t) {
//            if (t->get_op() == Op()) {
            logger.log(1, "op: {}", t->get_op());
            logger.log(1, "term: {}", t);
            logger.log(1, "term sort: {}", t->get_sort());

//            }
            if (t->is_symbol()) {
                preds.insert(t);
                return;
            }


            for (auto v: t) {
                logger.log(1, "dfs v: {}", v);
                dfs(v);
            }
        };

//        dfs(p);
        dfs(transitionSystem.trans());

        logger.log(defines::logFBMC, 1, "preds: ");
        for (auto v: preds) {
            logger.log(defines::logFBMC, 1, "v: {}", v);
        }

//        if (!p->is_symbol()) {
//            for (auto i: p) {
//                logger.log(defines::logFBMC, 2, "v = {}", i);
//                for (auto j : i) {
//
//                }
//            }
//        }
    }

}