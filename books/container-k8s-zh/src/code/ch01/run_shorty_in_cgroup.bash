#!/usr/bin/env bash
# 由 1_3_cgroup_memory_max.sh 调用：把当前 bash 的 PID 放进叶子 cgroup，再 exec shorty。
echo $$ > /sys/fs/cgroup/ch01demo/cgroup.procs
exec env SHORTY_LISTEN=127.0.0.1:18080 SHORTY_FAULT_LEAK_MB=8 GOMAXPROCS=1 /tmp/ch01-shorty serve
