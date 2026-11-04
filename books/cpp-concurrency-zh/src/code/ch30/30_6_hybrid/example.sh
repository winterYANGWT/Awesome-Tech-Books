#!/usr/bin/env bash
# needs: mpi
# flags: -fopenmp
# env: OMP_NUM_THREADS=2
# expect: requested=FUNNELED
# expect: omp_get_max_threads\(\)=2
# expect: 总校验和=799999980000000
set -euo pipefail

mpicxx $CXXFLAGS 30_6_hybrid.cpp -o "$BUILD/hybrid"
$MPIRUN -np 4 "$BUILD/hybrid"
