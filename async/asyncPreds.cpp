//
// Created by Morvan on 2022/11/5.
//

#include "asyncPreds.h"

#include <utility>

namespace wamcer {

    AsyncPreds::AsyncPreds(const smt::SmtSolver& predSolver) :
            slv(predSolver), to_slv(predSolver) {
        preds.emplace_back();
    }

    AsyncPreds::AsyncPreds(int size): slv(SolverFactory::boolectorSolver()), to_slv(slv) {
        resize(size);
    }

    void AsyncPreds::insert(const smt::Term& pred) {
        preds[0].insert(to_slv.transfer_term(pred));
    }

    int AsyncPreds::size() {
        auto lck = std::shared_lock(mux);
        return preds.size();
    }

    void AsyncPreds::map(const std::function<void(smt::Term)> &f, int index) {
        if (index > size()) {
            return;
        }
        auto lck = std::shared_lock(mux);
        preds[index].map(f);
    }

    bool AsyncPreds::empty() {
        auto lck = std::shared_lock(mux);
        return preds.empty();
    }

    smt::Term AsyncPreds::reduce(const std::function<smt::Term(smt::Term, smt::Term)>& f, smt::Term init, int index) {
        if (index > size()) {
            return init;
        }
        auto lck = std::shared_lock(mux);
        return preds[index].reduce(f, init);
    }

    void AsyncPreds::erase(const smt::TermVec& terms, int index) {
        if (index > size()) {
            return;
        }
        auto lck = std::shared_lock(mux);
        preds[index].erase(terms);
    }

    void AsyncPreds::pop(smt::Term& pred, int index) {
        if (index > size()) {
            return;
        }
        auto lck = std::shared_lock(mux);
        preds[index].pop(pred);
    }

    int AsyncPreds::size(int idx) {
        if (idx > size()) {
            throw std::runtime_error("index out of range in AsyncPreds::size");
        }
        auto lck = std::shared_lock(mux);
        return preds[idx].size();
    }

    void AsyncPreds::insert(const smt::TermVec& terms, int index) {
        {
            auto lck = std::unique_lock(mux);
            if (index >= preds.size()) {
                preds.resize(index + 1);
            }
        }
        auto lck = std::shared_lock(mux);
        preds[index].insert(terms);
    }

    void AsyncPreds::resize(int size) {
        auto lck = std::unique_lock(mux);
        preds.resize(size);
    }


}