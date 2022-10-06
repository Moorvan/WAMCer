#include "core/runner.h"
#include "core/solverFactory.h"
#include "engines/fbmc.h"

using namespace wamcer;
using namespace smt;

void Ints() {

}

int main() {
    auto s = SolverFactory::z3Solver();
    auto bv8 = s->make_sort(BV, 1);
    auto x = s->make_symbol("x", bv8);
    auto notX = s->make_term(BVNot, x);
    logger.log(0, "notX: {}", notX);
    logger.log(0, "notX.getop(): {}", notX->get_op());
    for (auto v: x) {
        logger.log(0, "v: {}", v);
        logger.log(0, "v->get_op(): {}", v->get_op());
    }
    return 0;
}