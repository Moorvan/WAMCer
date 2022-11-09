//
// Created by Morvan on 2022/10/29.
//

#include "DirectConstructor.h"

namespace wamcer {
    DirectConstructor::DirectConstructor(TransitionSystem &ts, Term &property, AsyncTermSet &preds, const SmtSolver& predSolver)
            : transitionSystem(ts), property(property), preds(preds), solver(ts.solver()), to_preds(predSolver) {}

    void DirectConstructor::generatePreds(int termRelationLvl, int complexPredsLvl) {
        this->termRelationLevel = termRelationLvl;
        this->complexPredsLevel = complexPredsLvl;
        collectStructuralPredicates();
        constructTermsRelation();
        constructComplexPreds();
    }

    void DirectConstructor::collectStructuralPredicates() {
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

        logger.log(defines::logDirectConstructor, 2, "base preds: ");
        for (auto v: basePreds) {
            logger.log(defines::logDirectConstructor, 2, "{}", v);
        }

    }

    void DirectConstructor::constructTermsRelation() {
        auto sortTerms = collectTerms();

        logger.log(defines::logDirectConstructor, 3, "add new term relations...");
        for (auto kvs: sortTerms) {
            for (auto v1: kvs.second) {
                for (auto v2: kvs.second) {
                    if (v1 != v2) {
                        auto v1EqV2 = solver->make_term(Equal, v1, v2);
                        if (termRelationLevel == 2) {
                            auto v1UltV2 = solver->make_term(BVUlt, v1, v2);
                            auto v1SltV2 = solver->make_term(BVSlt, v1, v2);
                            addToBasePreds({v1EqV2, v1UltV2, v1SltV2});
                        } else if (termRelationLevel == 1){
//                            logger.log(defines::logDirectConstructor, 2, "{}", v1EqV2);
                            addToBasePreds({v1EqV2});
                        }
                    }
                }
            }
        }
    }

    void DirectConstructor::constructComplexPreds() {

        // p
        for (auto t: basePreds) {
            preds.insert(to_preds.transfer_term(t));
        }

        // t1 /\ t2 -> t0
        if (complexPredsLevel >= 1) {
            for (auto t1: basePreds) {
                for (auto t2: basePreds) {
                    auto pred = solver->make_term(Implies, t1, t2);
                    preds.insert(to_preds.transfer_term(pred));
                }
            }
        }

        // t1 /\ t2 /\ t3 -> t0
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


    void DirectConstructor::addToBasePreds(TermVec terms) {
        for (auto t: terms) {
            basePreds.insert(t);
            basePreds.insert(solver->make_term(Not, t));
        }
    }

    SortTermSetMap DirectConstructor::collectTerms() {
        logger.log(defines::logDirectConstructor, 2, "collect terms...");

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
        logger.log(defines::logDirectConstructor, 2, "collect terms: ");
        for (auto kvs: sortTerms) {
            logger.log(defines::logDirectConstructor, 2, "   sort: {}", kvs.first);
            for (auto v: kvs.second) {
                logger.log(defines::logDirectConstructor, 2, "       term: {}", v);
                cnt++;
            }
        }
        logger.log(defines::logDirectConstructor, 2, "collect {} terms.", cnt);
        return sortTerms;
    }
}
