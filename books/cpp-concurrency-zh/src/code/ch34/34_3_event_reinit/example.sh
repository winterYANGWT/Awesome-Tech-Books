#!/usr/bin/env bash
# mode: run
# timeout: 30
# output: varies
set -euo pipefail

"$CC" $CFLAGS event_reinit.c -o "$BUILD/event_reinit" -levent

echo "== 调用 event_reinit =="
timeout 10 "$BUILD/event_reinit"
echo
echo "== 不调用 event_reinit =="
timeout 10 "$BUILD/event_reinit" --no-reinit
