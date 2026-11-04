#!/usr/bin/env bash
set -euo pipefail
# expect: transfer-a
mkdir -p "$BUILD"
# 编译到 $BUILD，再复制到临时目录：gdb 在某些沙箱里不能直接执行工作区里的文件。
$CXX $CXXFLAGS -O0 -fno-inline -o "$BUILD/deadlock" deadlock.cpp
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cp "$BUILD/deadlock" "$tmp/deadlock"

# gdb 启动程序；2 秒后向 gdb 发 SIGINT，让它在死锁现场停下来，
# 再执行 thread apply all bt。
timeout -s INT 3 gdb -batch -q \
    -ex "set pagination off" \
    -ex "set startup-with-shell off" \
    -ex "directory $PWD" \
    -ex run \
    -ex "thread apply all bt" \
    "$tmp/deadlock" > "$tmp/gdb.txt" 2>&1 || true

# 只保留程序自己的输出、线程标题，以及和锁/join/源码有关的帧。
awk '
/^Thread [0-9]+ .*received signal/ { print; next }
/^Thread [0-9]+/ {
    n = $2
    name = ""
    if (match($0, /"[^"]+"/)) {
        name = substr($0, RSTART, RLENGTH)
    }
    print "Thread " n " " name ":"
    next
}
/^transfer-|^主线程/ { print; next }
/^#[0-9]+/ {
    line = $0
    sub(/^#[0-9]+ +0x[0-9a-f]+ in /, "# ", line)
    sub(/ \(\) from .*/, "", line)
    sub(/\(anonymous namespace\)::/, "", line)
    if (line ~ /pthread_mutex_lock|std::thread::join|deadlock\.cpp:/) {
        print line
    }
}
' "$tmp/gdb.txt"
