# needs: mpi
# timeout: 30
# expect: 丢失任务数=1[\s\S]*Assertion
set -euo pipefail
: "${BUILD:?}" "${CXX:?}" "${CXXFLAGS:?}" "${MPIRUN:?}"
mkdir -p "$BUILD"

mpicxx $CXXFLAGS termination_bad.cpp -o "$BUILD/termination_bad"

set +e
timeout 15 $MPIRUN -np 3 "$BUILD/termination_bad" >"$BUILD/bad.log" 2>&1
rc=$?
set -e
if [ "$rc" -eq 0 ]; then
    echo "预期失败，但程序退出 0" >&2
    exit 1
fi
if ! grep -q "丢失任务数=1" "$BUILD/bad.log"; then
    echo "没有看到丢失任务数" >&2
    cat "$BUILD/bad.log" >&2
    exit 1
fi
tr -d '\0' <"$BUILD/bad.log" | grep -E 'rank 0|丢失任务数|Assertion' || true
echo "（子进程以退出码 $rc 结束）"
