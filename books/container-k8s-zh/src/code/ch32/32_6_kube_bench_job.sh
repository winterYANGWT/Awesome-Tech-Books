# 跑 CIS 检查：Job 必须能读节点上的配置和进程表。
kubectl apply -f kube-bench-job.yaml
#> timeout: 300
kubectl -n kube-bench wait --for=condition=Complete job/kube-bench --timeout=180s

# 报告在 Pod 日志里，先落到 lab 的文件，后面反复看不用再跑。
kubectl -n kube-bench logs job/kube-bench > /tmp/kube-bench.log

#> expect: \[FAIL\]|\[WARN\]
# 只看失败和警告项，前 15 条足够看出模式。
grep -E '^\[(FAIL|WARN)\]' /tmp/kube-bench.log | awk 'NR <= 15'

#> expect: FAIL=\d+ WARN=\d+
# 统计数量；kind 里出现 FAIL 是预期，不是脚本坏了。
awk '/^\[FAIL\]/{f++} /^\[WARN\]/{w++} END{printf "FAIL=%d WARN=%d\n", f, w}' /tmp/kube-bench.log
sed -e 's/\x1b\[[0-9;]*m//g' /tmp/kube-bench.log | grep -A8 'Summary'

# 按应用安全检查清单修一项：不需要 API 的默认身份不再自动挂 token。
kubectl -n shorty patch serviceaccount default --type=merge \
  -p '{"automountServiceAccountToken":false}'

#> expect: false
kubectl -n shorty get serviceaccount default \
  -o jsonpath='{.automountServiceAccountToken}{"\n"}'

#> expect: shorty-defaults
# 另一项是 32.4 已加的 LimitRange：新容器没写 requests/limits 时有默认值。
kubectl -n shorty get limitrange shorty-defaults
