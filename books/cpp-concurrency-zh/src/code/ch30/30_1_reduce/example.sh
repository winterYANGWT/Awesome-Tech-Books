#!/usr/bin/env bash
# needs: mpi
# expect: MPI_Reduce 到 root: 10
# expect: Allreduce 各 rank: 10 10 10 10
set -euo pipefail

mpicxx $CXXFLAGS 30_1_reduce.cpp -o "$BUILD/reduce"
$MPIRUN -np 4 "$BUILD/reduce"
