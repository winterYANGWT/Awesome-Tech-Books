#!/usr/bin/env bash
# needs: mpi
# expect: 矩阵第 0 列（按行）: 0 10 20 30
# expect: rank 1 收到并送回: 0 10 20 30
set -euo pipefail

mpicxx $CXXFLAGS 30_4_vector_type.cpp -o "$BUILD/vector_type"
$MPIRUN -np 4 "$BUILD/vector_type"
