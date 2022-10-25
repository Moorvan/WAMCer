//
// Created by Morvan on 2022/8/2.
//

#include "fbmc.h"


namespace wamcer {

    FBMC::FBMC(TransitionSystem &ts, Term &property, UnorderedTermSet &predicates, int &safeStep, std::mutex &mux,
               std::condition_variable &cv, TermTranslator &to_preds, int termRelationLevel, int complexPredsLevel) :
            preds(predicates),
            mux(mux),
            cv(cv),
            safeStep(safeStep),
            transitionSystem(ts),
            property(property),
            solver(ts.solver()),
            unroller(ts),
            to_preds(to_preds),
            to_solver(solver),
            termRelationLevel(termRelationLevel),
            complexPredsLevel(complexPredsLevel) {}

    bool FBMC::run(int bound) {
        logger.log(defines::logFBMC, 1, "init: {}", transitionSystem.init());
        logger.log(defines::logFBMC, 2, "trans: {}", transitionSystem.trans());
        logger.log(defines::logFBMC, 1, "prop: {}", property);

        collectStructuralPredicates();
        constructTermsRelation();
        constructComplexPreds();

        logger.log(defines::logFBMC, 2, "{} preds: ", preds.size());
        for (auto &pred : preds) {
            logger.log(defines::logFBMC, 2, "  {}", pred);
        }

        cv.notify_all();
        logger.log(defines::logFBMC, 2, "The initialization has {} preds.", preds.size());
        if (!step0()) {
            logger.log(defines::logFBMC, 0, "Check failed at init step.");
            return false;
        } else {
            logger.log(defines::logFBMC, 0, "Check safe at init step");
        }
        safeStep = 0;

        if (bound == 0) {
            return true;
        }

        for (int i = 1; i != bound; i++) {
            if (!stepN(i)) {
                logger.log(defines::logFBMC, 0, "Check failed at {} step.", i);
                return false;
            } else {
                logger.log(defines::logFBMC, 0, "Check safe at {} step.", i);
                safeStep = i;
            }
        }
        logger.log(defines::logFBMC, 0, "Safe in {} steps.", bound);
        return true;
    }

    void FBMC::collectStructuralPredicates() {
        auto bv1 = transitionSystem.make_sort(BV, 1);
        auto bool_ = transitionSystem.make_sort(BOOL);

        for (auto v: transitionSystem.statevars()) {
            if (v->get_sort() == bv1 or v->get_sort() == bool_) {
                addToBasePreds({v});
            }
        }

        std::function<void(Term)> dfs = [&](Term t) {
            auto hasOnlyCurSymbol = true;
            for (auto v: t) {
                if (v->get_op() != Op()) {
                    hasOnlyCurSymbol = false;
                    dfs(v);
                } else if (transitionSystem.is_next_var(v)) {
                    hasOnlyCurSymbol = false;
                }
            }
            if (hasOnlyCurSymbol) {
                if (t->get_sort() == bv1 or t->get_sort() == bool_) {
                    basePreds.insert(t);
                }
            }
        };

        dfs(transitionSystem.init());
        dfs(transitionSystem.trans());
        dfs(property);

        logger.log(defines::logFBMC, 2, "base preds: ");
        for (auto v: basePreds) {
            logger.log(defines::logFBMC, 2, "{}", v);
        }

    }


    bool FBMC::step0() {
        auto init0 = unroller.at_time(transitionSystem.init(), 0);
        solver->assert_formula(init0);
        auto prop0 = unroller.at_time(property, 0);
        auto bad0 = solver->make_term(Not, prop0);
        auto res = solver->check_sat_assuming({bad0});
        if (res.is_sat()) {
            logger.log(defines::logFBMC, 2, "init0 /\\ bad0 is sat.");
            return false;
        } else {
            logger.log(defines::logFBMC, 2, "init0 /\\ bad0 is unsat.");
            filterPredsAt(0);
            return true;
        }
    }

    bool FBMC::stepN(int n) {
        auto transNSub1 = unroller.at_time(transitionSystem.trans(), n - 1);
        solver->assert_formula(transNSub1);
        auto propN = unroller.at_time(property, n);
        auto badN = solver->make_term(Not, propN);
        auto res = solver->check_sat_assuming({badN});
        if (res.is_sat()) {
            logger.log(defines::logFBMC, 2, "init0 /\\ trans...{} /\\ bad{} is sat.", n, n);
            return false;
        } else {
            logger.log(defines::logFBMC, 2, "init0 /\\ trans...{} /\\ bad{} is unsat.", n, n);
            filterPredsAt(n);
            return true;
        }
    }

    void FBMC::filterPredsAt(int step) {
        auto notPass = TermVec();
        {
            auto lck = std::unique_lock(mux);
            for (auto v: preds) {
                auto slv_v = to_solver.transfer_term(v);
                auto v_at_step = unroller.at_time(slv_v, step);
                auto not_v = solver->make_term(Not, v_at_step);
                auto res = solver->check_sat_assuming({not_v});
                if (res.is_sat()) {
                    notPass.push_back(v);
                }
            }

            for (auto v: notPass) {
                preds.erase(v);
            }
        }
    }

    void FBMC::constructTermsRelation() {
        auto sortTerms = collectTerms();

        logger.log(defines::logFBMC, 3, "add new term relations...");
        for (auto kvs: sortTerms) {
            for (auto v1: kvs.second) {
                for (auto v2: kvs.second) {
                    if (v1 != v2) {
                        auto v1EqV2 = solver->make_term(Equal, v1, v2);
                        if (termRelationLevel == 1) {
                            auto v1UltV2 = solver->make_term(BVUlt, v1, v2);
                            auto v1SltV2 = solver->make_term(BVSlt, v1, v2);
                            addToBasePreds({v1EqV2, v1UltV2, v1SltV2});
                        } else {
                            logger.log(defines::logFBMC, 2, "{}", v1EqV2);
                            addToBasePreds({v1EqV2});
                        }
                    }
                }
            }
        }
    }

    SortTermSetMap FBMC::collectTerms() {
        logger.log(defines::logFBMC, 2, "collect terms...");

        auto sortTerms = SortTermSetMap();
        auto arrs = UnorderedTermSet();
        auto bv1 = transitionSystem.make_sort(BV, 1);

        std::function<void(Term)> dfs = [&](Term t) {
//            if (t->get_op() == Op() || t->get_op() == Select) {
            if (t->get_op() == Op()) {
                auto sort = t->get_sort();
                if (sort != bv1 && sort->get_sort_kind() == BV && transitionSystem.only_curr(t) && !t->is_value()) {
                    sortTerms[sort].insert(t);
                }
                if (t->get_sort()->get_sort_kind() == ARRAY && transitionSystem.only_curr(t)) {
                    arrs.insert(t);
                }
            } else {
                for (auto v: t) {
                    dfs(v);
                }
            }
        };

        dfs(transitionSystem.trans());
        dfs(transitionSystem.init());
        dfs(property);

        auto arrSortTerms = SortTermSetMap();
        for (auto arr: arrs) {
            auto idxSort = arr->get_sort()->get_indexsort();
            auto elemSort = arr->get_sort()->get_elemsort();
            for (auto idx: sortTerms[idxSort]) {
                auto term = solver->make_term(Select, arr, idx);
                arrSortTerms[elemSort].insert(term);
            }
        }

        for (auto kvs: arrSortTerms) {
            auto sort = kvs.first;
            for (auto v: kvs.second) {
                sortTerms[sort].insert(v);
            }
        }

        auto cnt = 0;
        logger.log(defines::logFBMC, 2, "collect terms: ");
        for (auto kvs: sortTerms) {
            logger.log(defines::logFBMC, 2, "   sort: {}", kvs.first);
            for (auto v: kvs.second) {
                logger.log(defines::logFBMC, 2, "       term: {}", v);
                cnt++;
            }
        }
        logger.log(defines::logFBMC, 2, "collect {} terms.", cnt);
        return sortTerms;
    }

    void FBMC::constructComplexPreds() {
        for (auto t: basePreds) {
            preds.insert(to_preds.transfer_term(t));
        }

        if (complexPredsLevel >= 1) {
            for (auto t1: basePreds) {
                for (auto t2: basePreds) {
                    auto pred = solver->make_term(Implies, t1, t2);
                    preds.insert(to_preds.transfer_term(pred));
                }
            }
        }

        if (complexPredsLevel >= 2) {
            for (auto t1: basePreds) {
                for (auto t2: basePreds) {
                    for (auto r: basePreds) {
                        auto t1AndT2 = solver->make_term(BVAnd, t1, t2);
                        auto pred = solver->make_term(Implies, t1AndT2, r);
                        preds.insert(to_preds.transfer_term(pred));
                    }
                }
            }
        }
    }

    void FBMC::addToBasePreds(TermVec terms) {
        for (auto t: terms) {
            basePreds.insert(t);
            basePreds.insert(solver->make_term(Not, t));
        }
    }
}