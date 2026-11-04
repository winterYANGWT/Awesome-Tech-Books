#!/usr/bin/env bash
set -euo pipefail
# expect: leaf
mkdir -p "$BUILD"
$CXX $CXXFLAGS -O0 -fno-inline -o "$BUILD/fork_demo" fork_demo.cpp
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
cp "$BUILD/fork_demo" "$tmp/fork_demo"

timeout 20 gdb -batch -q \
    -ex "set pagination off" \
    -ex "set follow-fork-mode child" \
    -ex "set detach-on-fork on" \
    -ex "set startup-with-shell off" \
    -ex "directory $PWD" \
    -ex "catch fork" \
    -ex "break leaf" \
    -ex run \
    -ex continue \
    -ex "bt" \
    -ex "info inferiors" \
    "$tmp/fork_demo" > "$tmp/gdb.txt" 2>&1

# 保留程序输出、fork/clone 的 catch、断点和子进程栈。
awk '
/^父进程|^子进程/ { print; next }
/^Catchpoint 1 \(forked process [0-9]+\)/ { print "Catchpoint 1 (fork -> child)"; next }
/^Catchpoint/ { print; next }
/^  Inferior|^  \*/ { print; next }
/^#[0-9]+/ {
    line = $0
    sub(/^#[0-9]+ +0x[0-9a-f]+ in /, "# ", line)
    sub(/ \(\) from .*/, "", line)
    if (line ~ /leaf|child_work|main|fork_demo\.cpp:/) {
        print line
    }
}
' "$tmp/gdb.txt"
