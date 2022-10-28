//
// Created by Morvan on 2022/10/27.
//

#include "asyncTermSet.h"

namespace wamcer {

    void AsyncTermSet::insert(smt::Term term) {
        auto lck = std::unique_lock(mux);
        data->insert(term);
    }

    void AsyncTermSet::erase(smt::Term term) {
        auto lck = std::unique_lock(mux);
        data->erase(term);
    }

    void AsyncTermSet::filter(std::function<bool(smt::Term)> f) {
        auto lck = std::unique_lock(mux);
        for (auto it = data->begin(); it != data->end();) {
            if (f(*it)) {
                it = data->erase(it);
            } else {
                ++it;
            }
        }
    }

    void AsyncTermSet::map(std::function<void(smt::Term)> f) {
        auto lck = std::shared_lock(mux);
        for (auto it = data->begin(); it != data->end(); ++it) {
            f(*it);
        }
    }

    smt::Term AsyncTermSet::reduce(std::function<smt::Term(smt::Term, smt::Term)> f, smt::Term init) {
        auto lck = std::shared_lock(mux);
        auto t = init;
        for (auto it = data->begin(); it != data->end(); ++it) {
            t = f(t, *it);
        }
        return t;
    }

    int AsyncTermSet::size() {
        auto lck = std::shared_lock(mux);
        return data->size();
    }

}