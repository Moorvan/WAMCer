//
// Created by Morvan on 2022/11/5.
//

#ifndef WAMCER_ASYNCPREDS_H
#define WAMCER_ASYNCPREDS_H

#include <utility>
#include "asyncTermSet.h"
#include "core/solverFactory.h"

namespace wamcer {
    class AsyncPreds {
    public:
        explicit AsyncPreds(const smt::SmtSolver& predSolver);

        void insert(const smt::Term& pred);

        bool empty();

        int size();

        void map(const std::function<void(smt::Term)>& f, int index);

        smt::Term reduce(const std::function<smt::Term(smt::Term, smt::Term)>& f, smt::Term init, int index);

        void erase(const smt::TermVec& terms, int index);

        void insert(const smt::TermVec& terms, int index);

    private:
        AsyncTermSetVec preds;
        std::shared_mutex mux;
        smt::SmtSolver slv;
        smt::TermTranslator to_slv;
    };
}


#endif //WAMCER_ASYNCPREDS_H
