# 24.7 排障顺序：先确认 CNI 在跑，再看策略对象和选择器，最后用新连接复现超时。
kubectl -n calico-system get daemonset calico-node
kubectl -n calico-system get pods -l k8s-app=calico-node

# 策略对象都在，名字和 policyTypes 一眼能看到。
kubectl -n shorty get networkpolicy

# worker 的出口只有 DNS、postgres 和 redis；api:8080 不在 allow 里，应该超时。
blocked=0
if timeout 12 kubectl -n shorty exec deploy/worker -- \
  python -c 'import socket; socket.create_connection(("api", 8080), timeout=3)' >/dev/null 2>&1; then
  echo "unexpected: worker -> api:8080 仍然可通"
else
  echo "timeout: worker -> api:8080 没有对应的 allow 规则"
  blocked=1
fi
[ "$blocked" -eq 1 ]
