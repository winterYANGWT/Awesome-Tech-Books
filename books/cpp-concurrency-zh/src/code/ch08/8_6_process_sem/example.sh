#!/usr/bin/env bash
# mode: run
# expect: 父进程: 子进程退出，counter = 1
set -euo pipefail

"$CC" $CFLAGS process_sem.c -o "$BUILD/process_sem"
"$BUILD/process_sem"
