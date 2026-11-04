#!/usr/bin/env bash
# mode: run
set -euo pipefail

"$CXX" $CXXFLAGS writer.cpp -o "$BUILD/writer"
"$CXX" $CXXFLAGS reader.cpp -o "$BUILD/reader"

name="/kvd-ch27-shm-$$"
cleanup() { rm -f "/dev/shm/${name#/}"; }
trap cleanup EXIT
cleanup

"$BUILD/writer" "$name"
echo "--- writer 已退出 ---"
"$BUILD/reader" "$name"
