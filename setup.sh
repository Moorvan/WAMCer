#!/bin/sh


cd smt-switch
  ./contrib/setup-btor.sh
  ./contrib/setup-bitwuzla.sh
  pip3 install toml
  ./contrib/setup-cvc5.sh
  ./contrib/setup-z3.sh
  ./configure.sh --btor --bitwuzla --cvc5 --z3 --prefix=local --static
  cd build
    make -j8
    make install
  # shellcheck disable=SC2103
  cd ..
cd ..