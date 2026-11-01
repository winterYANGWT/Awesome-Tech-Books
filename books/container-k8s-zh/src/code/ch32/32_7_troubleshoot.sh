#> expect: kube-bench
# kube-bench 的 Pod 在独立的 namespace 里，带 hostPID 和 hostPath。
kubectl -n kube-bench get pods -l job-name=kube-bench -o wide

#> expect: true
#> expect: etc-kubernetes=/etc/kubernetes
# 把它的关键字段单独打出来，确认不是“普通 Pod 却查不到结果”。
kubectl -n kube-bench get pod -l job-name=kube-bench -o jsonpath=\
'{.items[0].spec.hostPID}{"\n"}{range .items[0].spec.volumes[*]}{.name}={.hostPath.path}{"\n"}{end}'

#> expect: exists
# 节点上确实有 /etc/kubernetes，kube-bench 才有东西可读。
docker exec shorty-control-plane sh -c 'test -d /etc/kubernetes && echo /etc/kubernetes exists'

# 把同样的 hostPID/hostPath 放进 PSA restricted 的 shorty，准入会直接拒绝。
#> fails
#> expect: violates PodSecurity
kubectl apply -f hostpid-in-restricted.yaml

#> expect: --audit-policy-file
#> expect: audit.log
# 审计日志为空时，先确认 apiserver 真的带上了这两个参数。
docker exec shorty-control-plane grep -E 'audit-(policy-file|log-path)' \
  /etc/kubernetes/manifests/kube-apiserver.yaml
ls -lh /tmp/ch32-audit-log/audit.log

# apiserver 反复重启时，节点容器的日志里能看到 kubelet 拉起的静态 Pod 为什么退出。
docker logs shorty-control-plane 2>&1 | tail -n 4
