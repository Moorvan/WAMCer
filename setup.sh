#!/bin/sh


cd smt-switch
#  ./contrib/setup-btor.sh
#  ./contrib/setup-bitwuzla.sh
  ./configure.sh --btor --bitwuzla --prefix=local --static
  cd build
    make -j8
    make install
  # shellcheck disable=SC2103
  cd ..
cd ..