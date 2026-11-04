#!/usr/bin/env bash
# output: varies
# expect: OpenMP 执行流
set -euo pipefail

"$CXX" $CXXFLAGS std_thread.cpp -o "$BUILD/std_thread"
echo "== std::thread =="
"$BUILD/std_thread"

"$CC" $CFLAGS pthread_create.c -o "$BUILD/pthread_create"
echo "== pthread_create =="
"$BUILD/pthread_create"

"$CC" $CFLAGS -fopenmp omp_parallel.c -o "$BUILD/omp_parallel"
echo "== OpenMP =="
OMP_NUM_THREADS=4 "$BUILD/omp_parallel"
