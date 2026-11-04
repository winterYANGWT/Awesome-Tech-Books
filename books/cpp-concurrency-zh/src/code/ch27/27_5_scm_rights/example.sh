#!/usr/bin/env bash
# mode: run
set -euo pipefail

"$CC" $CFLAGS pass_fd.c -o "$BUILD/pass_fd"
payload="$BUILD/payload.txt"
rm -f "$payload"
trap 'rm -f "$payload"' EXIT

"$BUILD/pass_fd" "$payload"
