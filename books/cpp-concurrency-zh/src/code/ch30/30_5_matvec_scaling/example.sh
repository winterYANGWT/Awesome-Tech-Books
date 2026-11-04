#!/usr/bin/env bash
# needs: mpi
# output: varies
# flags: -O2
set -euo pipefail

n=8192
mkdir -p "$BUILD"

# 编译日志写 stderr，stdout 只留最终的 CSV。
mpicxx $CXXFLAGS matvec_mpi.cpp -o "$BUILD/matvec_mpi" 2>>"$BUILD/build.log"
$CXX $CXXFLAGS matvec_serial.cpp -o "$BUILD/matvec_serial" 2>>"$BUILD/build.log"

t1=$("$BUILD/matvec_serial" "$n" 2>>"$BUILD/run.log")
case "$t1" in
    ''|*[!0-9.]*)
        echo "串行时间无效: $t1" >&2
        exit 1
        ;;
esac

: > "$BUILD/rows.csv"
for p in 1 2 4 8; do
    line=$($MPIRUN -np "$p" "$BUILD/matvec_mpi" "$n" 2>>"$BUILD/run.log")
    printf '%s\n' "$line" >> "$BUILD/rows.csv"
done

# chart 只画加速比和效率；time_ms 留在 rows.csv 和 run.log 里，避免量纲差太多。
awk -F, -v t1="$t1" '
    BEGIN { print "p,speedup,efficiency" }
    {
        printf "%d,%.3f,%.3f\n", $1, t1 / $2, (t1 / $2) / $1
    }
' "$BUILD/rows.csv"
