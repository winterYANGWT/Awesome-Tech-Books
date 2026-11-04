#!/usr/bin/env bash
# needs: mpi
# expect: PID 各不相同：true
set -euo pipefail

mpicc $CFLAGS 1_5_mpi_rank.c -o "$BUILD/mpi_rank"
$MPIRUN -np 4 "$BUILD/mpi_rank"
