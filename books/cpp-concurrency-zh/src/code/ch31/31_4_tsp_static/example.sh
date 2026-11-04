# needs: mpi
# timeout: 60
# expect: 全局最优
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS tsp_static.cpp -o "$BUILD/tsp_static"
out=$($MPIRUN -np 4 "$BUILD/tsp_static")
printf '%s\n' "$out"
if ! grep -q "best_cost=338" <<<"$out"; then
    echo "best_cost 不是 338" >&2
    exit 1
fi
echo "全局最优：与串行暴力搜索的 338 一致"
