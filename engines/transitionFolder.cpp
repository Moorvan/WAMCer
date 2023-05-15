//
// Created by Morvan on 2022/11/9.
//

#include "transitionFolder.h"

#include <utility>

namespace wamcer {

    TransitionFolder::TransitionFolder(TransitionSystem &ts, const smt::SmtSolver &folderSolver)
            : slv(folderSolver), to_slv(folderSolver), ts(folderSolver) {
        this->ts = TransitionSystem(ts, to_slv);
        updates = this->ts.state_updates();
        this->trans.resize(2);
        maxTime = 1;
        original_inputs = this->ts.inputvars();
        this->trans[1] = this->ts.trans();
    }

    void TransitionFolder::getNStepTrans(int n, smt::Term &trans_out, smt::TermTranslator &translator) {
        if (n < trans.size() && trans[n] != nullptr) {
            trans_out = translator.transfer_term(trans[n]);
            return;
        }
        this->trans.resize(n + 1);
        auto out = TransitionSystem(slv);
        newTS(out);
        auto x = TransitionSystem(slv);
        newTS(x);
        x = add(x, ts, 0, 1);
//        x = ts;
        auto cur_x = 1, cur_out = 0;
        for (auto i = n; i > 0; i = i / 2) {
            logger.log(defines::logTransitionFolder, 1, "folding process {}/{}", n - i + 1, n);
            if (i % 2 != 0) {
                out = add(out, x, cur_out, cur_x);
                cur_out += cur_x;
                this->trans[cur_out] = out.trans();
            }
            if (i == 1) {
                break;
            }
            x = fold(x, cur_x);
            cur_x *= 2;
            if (cur_x < n) {
                this->trans[cur_x] = x.trans();
            }
        }
        trans_out = translator.transfer_term(out.trans());
    }

    void TransitionFolder::foldToNStep(int n, const std::function<void(int, const smt::Term &)> &add_trans) {
        auto out = TransitionSystem(slv);
        newTS(out);
        auto x = TransitionSystem(slv);
        newTS(x);
        x = add(x, ts, 0, 1);
        auto cur_x = 1, cur_out = 0;
        for (auto i = n; i > 0; i = i / 2) {
            logger.log(defines::logTransitionFolder, 2, "folding process {}/{}", n - i + 1, n);
            if (i % 2 != 0) {
                out = add(out, x, cur_out, cur_x);
                cur_out += cur_x;
                add_trans(cur_out, out.trans());
            }
            if (i == 1) {
                break;
            }
            x = fold(x, cur_x);
            cur_x *= 2;
            if (cur_x < n) {
                add_trans(cur_x, x.trans());
            }
        }
        add_trans(n, out.trans());
    }

    TransitionSystem TransitionFolder::fold(const TransitionSystem &in, int x) {
        addInputsTo(2 * x);
        auto out = TransitionSystem(slv);
        newTS(out);
        auto update = in.state_updates();
        for (auto &kv: update) {
            auto t = kv.second;
            t = substituteInput(x, x, t);
            update[kv.first] = t;
        }
        for (const auto &kv: in.state_updates()) {
            auto t = kv.second;
            auto foldTerm = slv->substitute(t, update);
            out.assign_next(kv.first, foldTerm);
        }
        return out;
    }

    TransitionSystem TransitionFolder::add(const TransitionSystem &in1, const TransitionSystem &in2, int x1, int x2) {
        addInputsTo(x1 + x2);
        auto out = TransitionSystem(slv);
        newTS(out);
        auto update = in2.state_updates();
        for (const auto &kv: update) {
            auto t = kv.second;
            t = substituteInput(x1, x2, t);
            update[kv.first] = t;
        }
        if (x1 != 0) {
            for (const auto &kv: in1.state_updates()) {
                auto foldTerm = slv->substitute(kv.second, update);
                out.assign_next(kv.first, foldTerm);
            }
        } else {
            for (const auto& kv : update) {
                out.assign_next(kv.first, kv.second);
            }
        }

        return out;
    }

    void TransitionFolder::newTS(TransitionSystem &out) {
        for (const auto &v: ts.statevars()) {
            out.add_statevar(v, ts.next(v));
        }
        for (const auto &v: ts.inputvars()) {
            out.add_inputvar(v);
        }
        for (const auto &v: ts.named_terms()) {
            out.name_term(v.first, v.second);
        }
    }

    smt::Term TransitionFolder::substituteInput(int start, int n, smt::Term in) {
        auto mp = smt::UnorderedTermMap();
        // i -> start + i
        for (auto i = 0; i < n; i++) {
            for (const auto &v: original_inputs) {
                mp[varAtTime(v, i)] = varAtTime(v, start + i);
            }
        }
        return slv->substitute(std::move(in), mp);
    }

    smt::Term TransitionFolder::varAtTime(smt::Term in, int time) {
        auto name = in->to_string() + "_" + std::to_string(time);
        if (time == 0) {
            return in;
        }
        if (time < maxTime) {
            return slv->get_symbol(name);
        }
        return slv->make_symbol(name, in->get_sort());
    }

    void TransitionFolder::addInputsTo(int x) {
        for (auto i = maxTime; i < x; i++) {
            for (const auto &v: original_inputs) {
                ts.add_inputvar(varAtTime(v, i));
            }
        }
        maxTime = std::max(maxTime, x);
    }
}