#!/usr/bin/env bash
# mode: hang
# timeout: 15
# output: varies
set -euo pipefail

"$CXX" $CXXFLAGS -O0 -g main.cpp -o "$BUILD/deadlock_gdb"
timeout --foreground 10 "$BUILD/deadlock_gdb" > "$BUILD/prog.log" 2>&1 &
tpid=$!
pid=""
cleanup() {
    if [ -n "$pid" ]; then kill "$pid" 2>/dev/null || true; fi
    kill "$tpid" 2>/dev/null || true
    wait "$tpid" 2>/dev/null || true
}
trap cleanup EXIT

sleep 1
cat "$BUILD/prog.log"
pid=$(pgrep -n -x deadlock_gdb || true)

# 先 info threads 看状态，再取所有线程的栈。只保留线程标题和帧，去掉地址。
if [ -n "$pid" ] \
   && timeout 5 gdb -p "$pid" -batch \
        -ex "set pagination off" \
        -ex "info threads" \
        -ex "thread apply all bt" > "$BUILD/gdb.log" 2>&1 \
   && ! grep -qi "ptrace\|Operation not permitted" "$BUILD/gdb.log" \
   && grep -q "^Thread" "$BUILD/gdb.log"; then
    # 只保留落在 main.cpp 的帧，按线程号排序，避免 libc 的 ?? 帧淹没源码行。
    awk '
        /^Thread [0-9]+ \(/ {
            tid = $2;
            gsub(/[^0-9]/, "", tid);
            block[tid] = "线程 " tid "：\n";
            if (tid + 0 > max) max = tid + 0;
            next;
        }
        /main\.cpp:[0-9]+/ { if (tid != "") block[tid] = block[tid] $0 "\n"; }
        END { for (t = 1; t <= max; ++t) if (t in block) printf "%s", block[t]; }
    ' "$BUILD/gdb.log" | sed -E 's/0x[0-9a-f]+/0x…/g'
else
    echo "（3 秒后仍未结束，强制终止）"
fi
