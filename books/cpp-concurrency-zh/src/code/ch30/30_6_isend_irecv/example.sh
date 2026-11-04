#!/usr/bin/env bash
# needs: mpi
# expect: rank 0 收到 30
# expect: rank 1 收到 0
# expect: rank 2 收到 10
# expect: rank 3 收到 20
set -euo pipefail

mpicxx $CXXFLAGS 30_6_isend_irecv.cpp -o "$BUILD/isend_irecv"
$MPIRUN -np 4 "$BUILD/isend_irecv"
