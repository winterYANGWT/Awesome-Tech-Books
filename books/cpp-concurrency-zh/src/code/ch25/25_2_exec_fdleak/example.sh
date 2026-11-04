#!/usr/bin/env bash
# mode: run
# timeout: 20
# expect: (?s)没有 CLOEXEC: 重新 bind -> Address already in use.*SOCK_CLOEXEC: 重新 bind -> ok
set -euo pipefail

"$CC" $CFLAGS exec_fdleak.c -o "$BUILD/exec_fdleak"
timeout 10 "$BUILD/exec_fdleak"
