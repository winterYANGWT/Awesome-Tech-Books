# needs: mpi
# timeout: 30
# expect: 环上等待[\s\S]*safe
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS ssend_ring.cpp -o "$BUILD/ssend_ring"
mpicxx $CXXFLAGS safe_ring.cpp -o "$BUILD/safe_ring"

set +e
out=$(timeout 5 $MPIRUN -np 4 "$BUILD/ssend_ring" 2>&1)
rc=$?
set -e
printf '%s\n' "$out" | grep -E '所有 rank|不应该' || true
if [ "$rc" -ne 124 ]; then
    echo "预期在 Ssend 上卡住，实际退出码 $rc" >&2
    exit 1
fi
echo "环上等待：所有 rank 停在 MPI_Ssend（timeout 5s）"

$MPIRUN -np 4 "$BUILD/safe_ring"
