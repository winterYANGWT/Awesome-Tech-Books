set -euo pipefail
# 只编 ASan 版：这个反例要靠 detect_stack_use_after_return 才稳定复现。
$CXX -std=c++20 -O1 -g -fsanitize=address -fno-omit-frame-pointer -pthread \
    36_4_coro_dangling.cpp -o "$BUILD/dangling"

set +e
ASAN_OPTIONS=detect_stack_use_after_return=1:detect_leaks=0:halt_on_error=1 \
    "$BUILD/dangling" >"$BUILD/run.log" 2>&1
rc=$?
set -e

head -n 6 "$BUILD/run.log"
grep -m1 "^SUMMARY" "$BUILD/run.log" || true
test "$rc" -ne 0
grep -q "stack-use-after-return" "$BUILD/run.log"
