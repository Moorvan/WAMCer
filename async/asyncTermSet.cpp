//
// Created by Morvan on 2022/10/27.
//

#include "asyncTermSet.h"

#include <utility>

namespace wamcer {

    AsyncTermSet::AsyncTermSet() {
        data = smt::UnorderedTermSet();
    }

    AsyncTermSet::AsyncTermSet(AsyncTermSet &&other) noexcept {
        data = other.data;
    }

    void AsyncTermSet::insert(const smt::Term& term) {
        auto lck = std::unique_lock(mux);
        data.insert(term);
    }

    void AsyncTermSet::insert(const smt::TermVec& terms) {
        auto lck = std::unique_lock(mux);
        for (auto& term : terms) {
            data.insert(term);
        }
    }

    void AsyncTermSet::erase(const smt::Term& term) {
        auto lck = std::unique_lock(mux);
        data.erase(term);
    }

    void AsyncTermSet::erase(const smt::TermVec& terms) {
        auto lck = std::unique_lock(mux);
        for (const auto& term : terms) {
            data.erase(term);
        }
    }

    void AsyncTermSet::filter(std::function<bool(smt::Term)> f) {
//        auto erases = smt::TermVec();
//        {
//            auto lck = std::shared_lock(mux);
//            for (auto term: data) {
//                if (f(term)) {
//                    erases.push_back(term);
//                }
//            }
//        }
//        auto lck = std::unique_lock(mux);
//        for (auto &term: erases) {
//            data.erase(term);
//        }
        auto lck = std::unique_lock(mux);
        for (auto it = data.begin(); it != data.end();) {
            if (f(*it)) {
                it = data.erase(it);
            } else {
                ++it;
            }
        }
    }

    void AsyncTermSet::map(const std::function<void(smt::Term)>& f) {
        auto lck = std::shared_lock(mux);
        for (const auto & it : data) {
            f(it);
        }
    }

    smt::Term AsyncTermSet::reduce(const std::function<smt::Term(smt::Term, smt::Term)>& f, smt::Term init) {
        auto lck = std::shared_lock(mux);
        auto t = std::move(init);
        for (const auto & it : data) {
            t = f(t, it);
        }
        return t;
    }

    int AsyncTermSet::size() {
        auto lck = std::shared_lock(mux);
        return data.size();
    }

}