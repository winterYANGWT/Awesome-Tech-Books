#!/usr/bin/env bash
# needs: mpi
# output: varies
# expect: n=1024 p=4 迭代=3
# expect: 校验和: [0-9]+
set -euo pipefail

mpicxx $CXXFLAGS 30_3_matvec.cpp -o "$BUILD/matvec"
$MPIRUN -np 4 "$BUILD/matvec"
