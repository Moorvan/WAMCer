#include "core/solverFactory.h"
#include "engines/fbmc.h"
#include "engines/DirectConstructor.h"
#include "engines/BMCChecker.h"
#include "utils/cmdline.h"
#include "utils/logger.h"
#include "frontends/btorSim.h"
#include "frontends/btor2_encoder.h"
#include "core/runner.h"

using namespace wamcer;
using namespace smt;
using namespace std;


void simulate() {
    auto parser = cmdline::parser();
    parser.add<string>("btor2", 'b', "btor file path", true);
    parser.add<string>("output", 'o', "output file path", false, "./out.csv");
    parser.add<int>("bound", 'k', "bound of simulation", false, 20);
//    parser.parse_check(argc, argv);
    auto btor2_path = parser.get<string>("btor2");
    auto output_path = parser.get<string>("output");
    auto bound = parser.get<int>("bound");
    auto slv = SolverFactory::boolectorSolver();
    // simulation
    sim::randomSim(btor2_path, slv, bound, int(time(nullptr)), output_path);

    // preds
    slv = SolverFactory::boolectorSolver();
    auto preds = AsyncTermSet();
    auto prop = Term();
    auto ts = TransitionSystem(slv);
    BTOR2Encoder::decoder(btor2_path, ts, prop);
    auto gen = DirectConstructor(ts, prop, preds, slv);
    gen.generatePreds(1, 0);
    // print the predset
    auto file = std::ofstream("./preds.txt");
    preds.map([&](auto t) {
//        logger.log(0, "{}", t);
        file << t << std::endl;
    });

    auto checker = BMCChecker(ts);
    auto check = [&](const Term &t, int bound) -> bool {
        for (auto i = 0; i < bound; ++i) {
            if (!checker.check(i, t)) {
                return false;
            }
        }
        return true;
    };
    if (check(prop, 20)) {
        logger.log(0, "safe");
    } else {
        logger.log(0, "unsafe");
    }

    // get map[string -> term]
    auto mp = ts.get_string_to_vars();
    for (auto &[k, v]: mp) {
        logger.log(0, "{} -> {}", k, v);
    }
}

/* main
    logger.set_verbosity(1);
    auto parser = cmdline::parser();
    parser.add<string>("btor2", 'b', "btor file path", true);
    parser.add<int>("bound", 'k', "bound of simulation", false, 25);
    parser.parse_check(argc, argv);
    auto btor2_path = parser.get<string>("btor2");
    auto bound = parser.get<int>("bound");
    auto res = Runner::runPredCP(btor2_path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, [](TransitionSystem &ts, Term &p, AsyncTermSet &preds, SmtSolver &s) {
        auto predsGen = new DirectConstructor(ts, p, preds, s);
        predsGen->generatePreds(1, 1);
    }, bound);
    if (res) {
        cout << "res: safe" << endl;
    } else {
        cout << "res: unsafe" << endl;
    }
 */

/* run all
 * using namespace std;
    logger.set_verbosity(2);
    auto btors = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/files.txt";
    // read lines in file btors
    ifstream ifs(btors);
    string line;
    while (getline(ifs, line)) {
        auto res = Runner::runBMCs(line, BTOR2Encoder::decoder, []() {
            return SolverFactory::boolectorSolver();
        }, 15, 5);
    }
    ifs.close();
 */

int main(int argc, char *argv[]) {
    logger.set_verbosity(1);
    auto parser = cmdline::parser();
    parser.add<string>("btor2", 'b', "btor file path", true);
    parser.add<string>("algorithm", 'a', "algorithm", false, "fbmc");
    parser.parse_check(argc, argv);
    auto btor2_path = parser.get<string>("btor2");
    auto algorithm = parser.get<string>("algorithm");

    if (algorithm == "fbmc") {
        auto res = Runner::runBMCWithFolder(btor2_path, BTOR2Encoder::decoder, []() {
            return SolverFactory::boolectorSolver();
        }, 26, 4, 5);
        if (res) {
            cout << "res: safe" << endl;
        } else {
            cout << "res: unsafe" << endl;
        }
    } else if (algorithm == "bmc") {
        auto path = btor2_path;
        auto s = SolverFactory::boolectorSolver();
        auto ts = TransitionSystem(s);
        auto p = Term();
        BTOR2Encoder::decoder_with_constraint(path, ts, p);
        auto init = ts.init();
        auto unroller = Unroller(ts);
        s->assert_formula(unroller.at_time(init, 0));
        auto f = [&](int t) {
            for (int i = 0; i < t; i++) {
                s->assert_formula(unroller.at_time(ts.trans(), i));
            }
            auto not_p = s->make_term(Not, p);
            assert(t > 0);
            auto not_p_s = s->make_term(false);
            for (int i = 1; i <= t; i++) {
                not_p_s = s->make_term(Or, not_p_s, unroller.at_time(not_p, i));
            }
            s->assert_formula(not_p_s);
            if (s->check_sat().is_unsat()) {
                logger.log(0, "pass in {} step", t);
            } else {
                logger.log(0, "Not pass in {} step", t);
            }
        };
        f(26);
    } else if (algorithm == "bmcs") {
        logger.set_verbosity(2);
        auto res = Runner::runBMCs(btor2_path, BTOR2Encoder::decoder, []() {
            return SolverFactory::boolectorSolver();
        }, 26, 5);
        if (res) {
            cout << "res: safe" << endl;
        } else {
            cout << "res: unsafe" << endl;
        }
    }
}