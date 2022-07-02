#include "frontends/btor2_encoder.h"
#include "smt-switch/bitwuzla_factory.h"
#include "engines/bmc.h"

using namespace wamcer;
using namespace smt;

int main() {
    logger.set_verbosity(2);
    auto path = "/Users/yuechen/Developer/clion-projects/WAMCer/btors/memory.btor2";
    auto ts = TransitionSystem();
    auto p = BTOR2Encoder(path, ts).propvec().at(0);
    auto bmc = BMC(ts, p);
    auto res = bmc.run(-1);
    logger.log(0, "prove result: {}", res);
    return 0;
}

