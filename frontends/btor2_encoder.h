/*********************                                                        */
/*! \file
 ** \verbatim
 ** Top contributors (to current version):
 **   Makai Mann, Ahmed Irfan
 ** This file is part of the pono project.
 ** Copyright (c) 2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved.  See the file LICENSE in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief
 **
 **
 **/

#pragma once


extern "C" {
#include "btor2parser/btor2parser.h"
}

#include <stdio.h>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include "assert.h"

#include "core/ts.h"
#include "utils/exceptions.h"
#include "utils/defines.h"

#include "smt-switch/smt.h"

namespace wamcer {
    class BTOR2Encoder {
    public:
        BTOR2Encoder(const std::string& filename, TransitionSystem &ts, bool constraints_merge_with_prop = false)
                : ts_(ts), solver_(ts.solver()) {
            preprocess(filename);
            if (constraints_merge_with_prop) {
                parse_without_constraint(filename);
            } else {
                parse(filename);
            }
        };

        const smt::TermVec &propvec() const { return propvec_; };

        smt::Term prop() const;

        smt::Term constraint() const;

        const smt::TermVec &justicevec() const { return justicevec_; };

        const smt::TermVec &fairvec() const { return fairvec_; };

        const smt::TermVec &inputsvec() const { return inputsvec_; }

        const smt::TermVec &statesvec() const { return statesvec_; }

        const std::map<uint64_t, smt::Term> &no_next_statevars() const {
            return no_next_states_;
        }

        static void decoder(const std::string& path, TransitionSystem& transitionSystem, smt::Term& prop);

        static void decode_without_constraint(const std::string& path, TransitionSystem& transitionSystem, smt::Term& prop);

    protected:
        // converts booleans to bitvector of size one
        smt::Term bool_to_bv(const smt::Term &t) const;

        // converts bitvector of size one to boolean
        smt::Term bv_to_bool(const smt::Term &t) const;

        // lazy conversion
        // takes a list of booleans / bitvectors of size one
        // and lazily converts them to the majority
        smt::TermVec lazy_convert(const smt::TermVec &) const;

        // preprocess a btor2 file
        void preprocess(const std::string &filename);

        // parse a btor2 file
        void parse(const std::string& filename);

        // parse a btor2 file without constraints
        void parse_without_constraint(const std::string& filename);

        // Important members
        const smt::SmtSolver &solver_;
        wamcer::TransitionSystem &ts_;

        // vectors of inputs and states
        // maintains the order from the btor file
        smt::TermVec inputsvec_;
        smt::TermVec statesvec_;
        std::map<uint64_t, smt::Term> no_next_states_;
        std::unordered_map<uint64_t, std::string> state_renaming_table;

        // Useful variables
        smt::Sort linesort_;
        smt::TermVec termargs_;
        std::unordered_map<int, smt::Sort> sorts_;
        std::unordered_map<int, smt::Term> terms_;
        std::string symbol_;

        smt::TermVec propvec_;
        smt::TermVec justicevec_;
        smt::TermVec fairvec_;
        smt::TermVec constraints_;

        Btor2Parser *reader_{};
        Btor2LineIterator it_{};
        Btor2Line *l_{};
        size_t i_{};
        int64_t idx_{};
        bool negated_{};
        size_t witness_id_{0};  ///< id of any introduced witnesses for properties
    };
}
