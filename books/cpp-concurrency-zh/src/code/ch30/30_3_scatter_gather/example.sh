#!/usr/bin/env bash
# needs: mpi
# expect: 原向量: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
# expect: 乘 2 后: 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30
set -euo pipefail

mpicxx $CXXFLAGS 30_3_scatter_gather.cpp -o "$BUILD/scatter_gather"
$MPIRUN -np 4 "$BUILD/scatter_gather"
