//
// Created by Morvan on 2022/10/27.
//

#ifndef WAMCER_ASYNCTERMSET_H
#define WAMCER_ASYNCTERMSET_H

#include <shared_mutex>
#include "smt-switch/smt.h"

namespace wamcer {
    class AsyncTermSet {
    public:
        AsyncTermSet();

        AsyncTermSet(AsyncTermSet &&other) noexcept;

        void insert(const smt::Term& term);

        void insert(const smt::TermVec& terms);

        void erase(const smt::Term& term);

        void erase(const smt::TermVec& terms);

        void filter(std::function<bool(smt::Term)> filter);

        void map(const std::function<void(smt::Term)>& map);

        smt::Term reduce(const std::function<smt::Term(smt::Term, smt::Term)>& f, smt::Term init);

        int size();


    private:
        std::shared_mutex mux;
        smt::UnorderedTermSet data;
    };

    using AsyncTermSetVec = std::vector<AsyncTermSet>;
}


#endif //WAMCER_ASYNCTERMSET_H
