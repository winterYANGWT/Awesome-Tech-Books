# 16.7 kind 创建失败时先确认 daemon、磁盘和内存，而不是反复重试。
docker info --format 'Docker {{.ServerVersion}} / {{.Driver}} / cgroup v{{.CgroupVersion}}'
df -h /var/lib/docker | tail -1
free -m
printf 'cgroup memory.max=%s current=%s\n' \
  "$(cat /sys/fs/cgroup/memory.max)" "$(cat /sys/fs/cgroup/memory.current)"

# 当前 context 必须是 kind-shorty；不是就重新导出 kubeconfig。
kubectl config current-context
kind export kubeconfig --name shorty
kubectl config current-context

# 模拟一个指向错误 API server 的旧 context，kubectl 连不上。
kubectl config set-cluster kind-old --server=https://127.0.0.1:1 --insecure-skip-tls-verify=true
kubectl config set-context kind-old --cluster=kind-old --user=kind-shorty
kubectl config use-context kind-old
kubectl config current-context
#> fails
#> expect: refused
kubectl get nodes --request-timeout=3s

# 回到 kind export 写出的 context，节点仍然 Ready。
kind export kubeconfig --name shorty
kubectl config current-context
kubectl get nodes
kind get clusters
