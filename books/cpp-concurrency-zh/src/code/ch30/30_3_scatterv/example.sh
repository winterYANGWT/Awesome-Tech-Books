#!/usr/bin/env bash
# needs: mpi
# expect: 各 rank 收到: 3 3 2 2
# expect: 拼接后: 0 1 2 3 4 5 6 7 8 9
set -euo pipefail

mpicxx $CXXFLAGS 30_3_scatterv.cpp -o "$BUILD/scatterv"
$MPIRUN -np 4 "$BUILD/scatterv"
