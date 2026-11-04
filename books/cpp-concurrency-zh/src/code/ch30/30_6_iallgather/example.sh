#!/usr/bin/env bash
# needs: mpi
# output: varies
# expect: 校验和: [0-9.]+
# expect: 阻塞版: [0-9.]+ ms
# expect: 非阻塞版: [0-9.]+ ms
set -euo pipefail

mpicxx $CXXFLAGS 30_6_iallgather.cpp -o "$BUILD/iallgather"
$MPIRUN -np 4 "$BUILD/iallgather"
