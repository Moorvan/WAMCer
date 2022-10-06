#include "core/runner.h"
#include "core/solverFactory.h"
#include "engines/fbmc.h"

using namespace wamcer;
using namespace smt;

bool f(std::string path, void (*decoder)(std::string, TransitionSystem &, Term &),
       smt::SmtSolver (*solverFactory)(), int bound) {
    logger.log(defines::logFBMCKindRunner, 0, "file: {}", path);
    logger.log(defines::logFBMCKindRunner, 0, "FBMC + K-Induction running...");
    auto safe = int();
    auto mux = std::mutex();
    auto cv = std::condition_variable();
    auto finish = std::condition_variable();
    auto res = std::atomic<bool>();
    auto isFinished = false;

    auto toPredSolver = SolverFactory::boolectorSolver();
    auto preds = UnorderedTermSet();

    auto bmcSlv = solverFactory();
    auto ts = TransitionSystem(bmcSlv);
    auto p = Term();
    decoder(path, ts, p);

    auto bmcRun = std::thread([&] {
//            auto bv8 = bmcSlv->make_sort(BV, 8);
//            auto x = bmcSlv->make_symbol("x", bv8);
//            auto y = bmcSlv->make_symbol("y", bv8);
//            preds.insert(toPred.transfer_term(x));
        auto toPred = TermTranslator(toPredSolver);
        auto fbmc = FBMC(ts, p, preds, safe, mux, cv, toPred);
        auto fbmcRes = fbmc.run(bound);
        finish.notify_all();
//            if (fbmcRes) {
//                logger.log(defines::logFBMC, 0, "Result: safe in {} steps.", bound);
//            } else {
//                logger.log(defines::logFBMC, 0, "Result: unsafe.");
//                res = false;
//                finish.notify_all();
//            }
    });
    {
        auto lck = std::unique_lock(mux);
        finish.wait(lck);
    }
    bmcRun.join();
    auto kind_slv = SolverFactory::boolectorSolver();
    auto kind_ts = TransitionSystem(kind_slv);
    auto kind_prop = BTOR2Encoder(path, kind_ts).propvec().at(0);
    auto to_kind_slv = TermTranslator(kind_slv);
    logger.log(1, "old_prop = {}", kind_prop);
    for (auto v: preds) {
        kind_prop = kind_slv->make_term(And, kind_prop, to_kind_slv.transfer_term(v));
    }
    logger.log(1, "new_prop = {}", kind_prop);
}


int main() {
    logger.set_verbosity(2);
    auto path = "../btors/memory.btor2";
//    Runner::runFBMCWithKInduction(path, BTOR2Encoder::decoder, []() {
//        return SolverFactory::boolectorSolver();
//    }, 9);
    f(path, BTOR2Encoder::decoder, []() {
        return SolverFactory::boolectorSolver();
    }, 9);
    return 0;
}