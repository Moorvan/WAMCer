//
// Created by Morvan on 2022/7/6.
//

#ifndef WAMCER_COUNTER_H
#define WAMCER_COUNTER_H

#include "utils/logger.h"
#include "core/ts.h"

using namespace smt;

namespace wamcer {
    void counter(TransitionSystem &ts, Term &property);
}

#endif //WAMCER_COUNTER_H
