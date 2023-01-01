#include "core/solverFactory.h"
#include "engines/fbmc.h"
#include "engines/DirectConstructor.h"
#include "engines/BMCChecker.h"
#include "utils/cmdline.h"
#include "utils/logger.h"
#include "frontends/btorSim.h"
#include "frontends/btor2_encoder.h"

using namespace wamcer;
using namespace smt;
using namespace std;


int main(int argc, char *argv[]) {
    auto parser = cmdline::parser();
    parser.add<string>("btor2", 'b', "btor file path", true);
    parser.add<string>("output", 'o', "output file path", false, "./out.csv");
    parser.add<int>("bound", 'k', "bound of simulation", false, 20);
    parser.parse_check(argc, argv);
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
    // print the predse
    auto file = std::ofstream("./preds.txt");
    preds.map([&](auto t) {
//        logger.log(0, "{}", t);
        file << t << std::endl;
    });

    auto checker = BMCChecker(ts);
    if (checker.check(10, prop)) {
        logger.log(0, "safe");
    } else {
        logger.log(0, "unsafe");
    }

    // get map[string -> term]
    auto mp = ts.get_string_to_vars();
    for (auto& [k, v] : mp) {
        logger.log(0, "{} -> {}", k, v);
    }

    return 0;
}