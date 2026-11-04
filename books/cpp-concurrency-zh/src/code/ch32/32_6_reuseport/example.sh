#!/usr/bin/env bash
# mode: run
# timeout: 30
# output: varies
set -euo pipefail

"$CC" $CFLAGS reuseport.c -o "$BUILD/reuseport"
timeout 20 "$BUILD/reuseport"
