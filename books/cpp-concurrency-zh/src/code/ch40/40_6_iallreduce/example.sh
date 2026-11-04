#!/usr/bin/env bash
# needs: mpi
# output: varies
# expect: mpi_version=3.1
# expect: allreduce_ok=1
# expect: blocking_ms=[0-9.]+
# expect: overlap_ms=[0-9.]+
set -euo pipefail

mpicxx $CXXFLAGS 40_6_iallreduce.cpp -o "$BUILD/iallreduce"
$MPIRUN -np 4 "$BUILD/iallreduce"
