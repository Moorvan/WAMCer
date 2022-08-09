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

        auto atomicPreds = UnorderedTermSet();
        auto bv1 = transitionSystem.make_sort(BV, 1);
        auto bool_ = transitionSystem.make_sort(BOOL);

        std::function<void(Term)> dfs = [&](Term t) {
            if (t->get_op() == Op()) {
                if (t->get_sort() == bv1 or t->get_sort() == bool_) {
                    atomicPreds.insert(t);
                }
                return;
            }

            auto hasOnlySymbol = true;
            for (auto v: t) {
                if (v->get_op() != Op()) {
                    hasOnlySymbol = false;
                }
                dfs(v);
            }
            if (hasOnlySymbol) {
                if (t->get_sort() == bv1 or t->get_sort() == bool_) {
                    atomicPreds.insert(t);
                }
            }
        };

        dfs(transitionSystem.init());
        dfs(transitionSystem.trans());
        dfs(property);

        logger.log(defines::logFBMC, 1, "atomicPreds: ");
        for (auto v: atomicPreds) {
            logger.log(defines::logFBMC, 1, "v: {}", v);
            preds.insert(to_preds.transfer_term(v));
        }
    }

}