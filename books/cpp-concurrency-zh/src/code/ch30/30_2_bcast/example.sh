#!/usr/bin/env bash
# needs: mpi
# expect: 两次结果一致: true
set -euo pipefail

mpicxx $CXXFLAGS 30_2_bcast.cpp -o "$BUILD/bcast"
$MPIRUN -np 4 "$BUILD/bcast"
