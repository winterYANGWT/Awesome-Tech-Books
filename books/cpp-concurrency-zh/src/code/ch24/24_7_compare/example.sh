#!/usr/bin/env bash
set -euo pipefail
# mode: run
# output: varies
# timeout: 120

BUILD="${BUILD:-build}"
mkdir -p "$BUILD"
LOG="$BUILD/build.log"
: >"$LOG"
trap 'cat "$LOG" >&2' ERR

$CC $CFLAGS nbody_pthread.c -o "$BUILD/pthread" >>"$LOG" 2>&1
$CXX $CXXFLAGS -fopenmp nbody_omp.cpp -o "$BUILD/openmp" >>"$LOG" 2>&1
$CXX $CXXFLAGS nbody_stdthread.cpp -o "$BUILD/stdthread" >>"$LOG" 2>&1

"$BUILD/pthread" >"$BUILD/pthread.csv" 2>>"$LOG"
"$BUILD/openmp" >"$BUILD/openmp.csv" 2>>"$LOG"
"$BUILD/stdthread" >"$BUILD/stdthread.csv" 2>>"$LOG"

python3 - "$BUILD" 2>>"$LOG" <<'PY'
import csv
import sys
from pathlib import Path

build = Path(sys.argv[1])


def load(name):
    with (build / name).open(newline="") as f:
        rows = list(csv.DictReader(f))
    return {int(r["threads"]): float(r["ms"]) for r in rows}


pthread = load("pthread.csv")
openmp = load("openmp.csv")
stdthread = load("stdthread.csv")
assert pthread.keys() == openmp.keys() == stdthread.keys()

print("threads,pthread_speedup,openmp_speedup,stdthread_speedup")
for t in sorted(pthread):
    print(f"{t},{pthread[1] / pthread[t]:.3f},"
          f"{openmp[1] / openmp[t]:.3f},{stdthread[1] / stdthread[t]:.3f}")
PY
