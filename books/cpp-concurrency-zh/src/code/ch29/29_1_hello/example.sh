#!/usr/bin/env bash
# needs: mpi
# expect: MPI 世界：4 个进程
set -euo pipefail

mpicc $CFLAGS hello.c -o "$BUILD/hello"
$MPIRUN -np 4 "$BUILD/hello"
