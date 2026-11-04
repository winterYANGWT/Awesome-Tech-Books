#!/usr/bin/env bash
# needs: mpi
# expect: 汇总：40.0
set -euo pipefail

mpicc $CFLAGS input.c -o "$BUILD/input"
printf '1.5 2.5 4\n' | $MPIRUN -np 4 "$BUILD/input"
