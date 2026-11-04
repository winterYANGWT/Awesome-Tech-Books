set -euo pipefail
# expect: 父进程：mutex 已被 worker 持有，现在 fork
# expect: 子进程：准备对同一把锁加锁
# expect: pthread_mutex_lock
$CC $CFLAGS fork_thread_deadlock.c -o "$BUILD/fork_thread_deadlock"
tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT
cp "$BUILD/fork_thread_deadlock" "$tmpdir/deadlock"
echo "2 秒后用 gdb 跟上 fork 出的子进程，取卡住时的栈："
timeout 15 gdb -q -batch \
  -ex 'set pagination off' \
  -ex 'set follow-fork-mode child' \
  -ex 'set detach-on-fork on' \
  -ex 'break on_alarm' \
  -ex run \
  -ex 'thread apply all bt 8' \
  -ex 'kill' \
  --args "$tmpdir/deadlock" 2>&1 \
  | awk '/^Thread .*\(Thread/{print "子进程（只剩调用线程）："; next} /^#[0-9]+/{print}' | head -12 || true
