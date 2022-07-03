//
// Created by Morvan on 2022/7/2.
//

#include "k_induction.h"
#include <thread>
#include <chrono>


namespace wamcer {
    KInduction::KInduction(TransitionSystem &ts, Term &property, int &safeBound) :
            transitionSystem(ts),
            solver(ts.solver()),
            property(property),
            unroller(ts),
            safeBound(safeBound){}

    bool KInduction::run() {
        while (true) {
            logger.log(0, "safe bound: {}", safeBound);
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
    }
}