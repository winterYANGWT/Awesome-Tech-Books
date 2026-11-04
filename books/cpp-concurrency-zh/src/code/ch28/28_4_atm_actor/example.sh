#!/usr/bin/env bash
set -euo pipefail
# mode: run
# timeout: 20
# expect: 驱动完成：8 条界面消息，3 个 actor 都已汇入
mkdir -p "$BUILD"
$CXX $CXXFLAGS -pthread main.cpp actors.cpp -o "$BUILD/atm_actor"
"$BUILD/atm_actor"
