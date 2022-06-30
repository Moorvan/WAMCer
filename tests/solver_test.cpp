//
// Created by 项慕凡 on 2022/6/30.
//
#include <gtest/gtest.h>
#include "smt-switch/bitwuzla_factory.h"
using namespace smt;

TEST(solverTest, Ass) {
    auto s = BitwuzlaSolverFactory::create(true);
}