#!/usr/bin/env bash
# needs: mpi
# expect: 普通 Reduce: 10
# expect: IN_PLACE Reduce: 10
# expect: 两种写法一致: true
set -euo pipefail

mpicxx $CXXFLAGS 30_1_in_place.cpp -o "$BUILD/in_place"
$MPIRUN -np 4 "$BUILD/in_place"
