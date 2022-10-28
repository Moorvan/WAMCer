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

//        ~AsyncTermSet() {
//            delete data;
//        }

        void insert(smt::Term term);

        void erase(smt::Term term);

        void filter(std::function<bool(smt::Term)> filter);

        void map(std::function<void(smt::Term)> map);

        smt::Term reduce(std::function<smt::Term(smt::Term, smt::Term)> f, smt::Term init);

        int size();


    private:
        std::shared_mutex mux;
        smt::UnorderedTermSet* data;
    };
}


#endif //WAMCER_ASYNCTERMSET_H
