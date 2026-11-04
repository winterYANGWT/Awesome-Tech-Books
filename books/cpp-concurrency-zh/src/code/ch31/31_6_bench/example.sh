# needs: mpi
# timeout: 180
# output: varies
# expect: procs,sort_speedup,nbody_speedup,tsp_speedup
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"
: >"$BUILD/build.log"
: >"$BUILD/run.log"

for src in bench_sort bench_nbody bench_tsp; do
    mpicxx $CXXFLAGS "$src.cpp" -o "$BUILD/$src" 2>>"$BUILD/build.log"
done

detail="$BUILD/detail.csv"
printf 'case,procs,time_ms,checksum\n' >"$detail"

run_one() {
    local exe=$1 np=$2 out
    if ! out=$($MPIRUN -np "$np" "$BUILD/$exe" 2>>"$BUILD/run.log"); then
        echo "运行失败：$exe np=$np" >&2
        cat "$BUILD/run.log" >&2
        exit 1
    fi
    printf '%s\n' "$out"
}

get_time() { sed -n 's/.*time_ms=\([0-9.]*\).*/\1/p' <<<"$1"; }
get_sum()  { sed -n 's/.*checksum=\([^ ]*\).*/\1/p' <<<"$1"; }

echo "procs,sort_speedup,nbody_speedup,tsp_speedup"
s1=""; n1=""; t1=""
for np in 1 2 4 8; do
    ls=$(run_one bench_sort "$np");  ts=$(get_time "$ls"); cs=$(get_sum "$ls")
    ln=$(run_one bench_nbody "$np"); tn=$(get_time "$ln"); cn=$(get_sum "$ln")
    lt=$(run_one bench_tsp "$np");   tt=$(get_time "$lt"); ct=$(get_sum "$lt")

    printf 'sort,%d,%s,%s\n'  "$np" "$ts" "$cs" >>"$detail"
    printf 'nbody,%d,%s,%s\n' "$np" "$tn" "$cn" >>"$detail"
    printf 'tsp,%d,%s,%s\n'   "$np" "$tt" "$ct" >>"$detail"

    if [ "$np" -eq 1 ]; then
        s1=$ts; n1=$tn; t1=$tt
    fi
    awk -v p="$np" -v s="$ts" -v n="$tn" -v t="$tt" \
        -v s1="$s1" -v n1="$n1" -v t1="$t1" \
        'BEGIN { printf "%d,%.3f,%.3f,%.3f\n", p, s1/s, n1/n, t1/t }'
done
