#!/usr/bin/env bash
# mode: run
# timeout: 20
set -euo pipefail

"$CXX" $CXXFLAGS main.cpp -o "$BUILD/replace"
timeout 10 "$BUILD/replace"
