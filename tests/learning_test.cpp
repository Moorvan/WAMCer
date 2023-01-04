//
// Created by Morvan on 2023/1/3.
//

#include <gtest/gtest.h>
#include "utils/logger.h"
#include "core/solverFactory.h"
#include "engines/fbmc.h"
#include "engines/DirectConstructor.h"
#include "engines/BMCChecker.h"
#include "frontends/btorSim.h"
#include "engines/InductionProver.h"

using namespace wamcer;
using namespace std;

TermVec read_from_file(const string &path, SmtSolver &slv, unordered_map<string, Term> term_map) {
    auto trim = [](string &s, char del=' ') {
        s.erase(0, s.find_first_not_of(del));
        s.erase(s.find_last_not_of(del) + 1);
    };

    auto split = [&](const string &s, char delim) -> vector<string> {
        vector<string> elems;
        stringstream ss(s);
        string item;
        while (getline(ss, item, delim)) {
            trim(item);
            elems.push_back(item);
        }
        return elems;
    };

    auto slip_first = [&](const string &s, char delim) -> string {
        auto pos = s.find(delim);
        if (pos == string::npos) {
            return s;
        }
        return s.substr(0, pos);
    };

    // get subterm from term list. elem may in a pair of () or single, the split by space
    // example: input: (not (= (select buffer rdptr) (select buffer wrptr))); output: "(not (= (select buffer rdptr) (select buffer wrptr)))" only one term
    // example: input: (= out_rdata (select buffer wrptr)); output "(= out_rdata (select buffer wrptr))" only one term
    auto get_subterm_str_s = [&](const string& s) -> vector<string> {
        auto ss = s + " ";
        vector<string> ret;
        int left = 0;
        int right = 0;
        int start = 0;
        for (int i = 0; i < ss.size(); i++) {
            if (ss[i] == '(') {
                left++;
            } else if (ss[i] == ')') {
                right++;
            } else if (ss[i] == ' ') {
                if (left == right) {
                    auto new_s = ss.substr(start, i - start);
                    trim(new_s);
                    if (!new_s.empty()) {
                        ret.push_back(new_s);
                    }
                    start = i + 1;
                }
            }
        }
        return ret;
    };

    function<Term(const string&)> parse_term = [&](const string &s) -> Term {
        auto ss = s;
        if (s[0] == '(') {
            ss = s.substr(1, s.size() - 2);
        }
        // if ss has no space, it is a variable or a constant
        if (ss.find(' ') == string::npos) {
            if (ss.find("#b") == 0) {
                auto sss = ss.substr(2);
                auto len = sss.size();
                auto val = stoi(sss, nullptr, 2);
                return slv->make_term(val, slv->make_sort(BV, len));
            }
            if (term_map.find(ss) == term_map.end()) {
                auto msg = "Cannot find term " + ss + " in term_map";
                throw std::runtime_error(msg);
            }
            return term_map[ss];
        }

        auto op = slip_first(ss, ' ');
        if (op == "=") {
            auto sub_term_str_s = get_subterm_str_s(ss.substr(op.size()));
//            for (auto &sub_term_str : sub_term_str_s) {
//                logger.log(0, "sub_term_str:{}", sub_term_str);
//            }
            if (sub_term_str_s.size() != 2) {
                throw std::runtime_error("Invalid term: " + s);
            }
            return slv->make_term(Equal, parse_term(sub_term_str_s[0]), parse_term(sub_term_str_s[1]));
        } else if (op == "select") {
            auto sub_term_str_s = get_subterm_str_s(ss.substr(op.size()));
//            for (auto &sub_term_str : sub_term_str_s) {
//                logger.log(0, "sub_term_str:{}", sub_term_str);
//            }
            if (sub_term_str_s.size() != 2) {
                throw std::runtime_error("Invalid term: " + s);
            }
            return slv->make_term(Select, parse_term(sub_term_str_s[0]), parse_term(sub_term_str_s[1]));
        } else if (op == "not" || op == "bvnot") {
            auto sub_term_str_s = get_subterm_str_s(ss.substr(op.size()));
//            for (auto &sub_term_str : sub_term_str_s) {
//                logger.log(0, "sub_term_str:{}", sub_term_str);
//            }
            if (sub_term_str_s.size() != 1) {
                throw std::runtime_error("Invalid term: " + s);
            }
            return slv->make_term(BVNot, parse_term(sub_term_str_s[0]));
        } else {
            throw std::runtime_error("Invalid term: " + s + "unknown op: " + op);
        }
    };

    auto parse = [&](const string &s) -> Term {
        auto term_str_s = split(s, '&');
        auto terms = TermVec();
        for (auto &term_str : term_str_s) {
//            cout << "term_str: " << term_str << endl;
            terms.emplace_back(parse_term(term_str));
        }
        if (terms.size() == 1) {
            return terms[0];
        }
        return slv->make_term(And, terms);
    };

//    auto t = parse("(bvnot state40) & (not (= (select buffer rdptr) (select buffer wrptr)))");
//    logger.log(0, "t = {}", t->to_string());
    auto file = ifstream(path);
    auto res = TermVec();
    while (!file.eof()) {
        auto line = string();
        getline(file, line);
        if (line.empty()) {
            continue;
        }
//        logger.log(0, "line: {}", line);
        auto pos = line.find(":");
        auto term_str = line.substr(pos + 1);
        pos = term_str.find("->");
        auto left = term_str.substr(0, pos);
        trim(left);
        auto right = term_str.substr(pos + 2);
        trim(right);
//        logger.log(0, "term: {}", slv->make_term(Implies, parse(left), parse(right))->to_string());
        res.emplace_back(slv->make_term(Implies, parse(left), parse(right)));
    }
    return res;
}

TEST(CaseLearning, Buffer) {
    auto btor2_path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/buffer.btor2";
    auto slv = SolverFactory::boolectorSolver();
    auto prop = Term();
    auto ts = TransitionSystem(slv);
    BTOR2Encoder::decoder(btor2_path, ts, prop);
    auto mp = ts.get_string_to_vars();

    auto terms = read_from_file("/Users/yuechen/Documents/caches/2023.1/invariants.txt", slv, mp);

    // check and prove
    logger.log(0, "terms size: {}", terms.size());

    // bmc check 20 steps
    slv->push();
    auto checker = BMCChecker(ts);
    for (auto i = 0; i < 50; i++) {
        auto erase_terms = TermVec();
        for (auto &t : terms) {
            if (!checker.check(i, t)) {
                erase_terms.push_back(t);
            }
        }
        for (auto &t : erase_terms) {
            terms.erase(std::remove(terms.begin(), terms.end(), t), terms.end());
        }

        logger.log(0, "terms size: {}", terms.size());
        if (checker.check(i, prop)) {
            logger.log(0, "property holds at step {}", i);
        } else {
            logger.log(0, "property does not hold at step {}", i);
        }
    }
    slv->pop();

    auto term = slv->make_term(And, terms);
    // k-induction prove: prop /\ terms
    auto prover = InductionProver(ts, prop);
    for (auto i = 1; i <= 50; i++) {
        if (prover.prove(i, term)) {
            logger.log(0, "proved is {} step invariant.", i);
            return;
        } else {
            logger.log(0, "{} step invariant prove failed", i);
        }
    }
}