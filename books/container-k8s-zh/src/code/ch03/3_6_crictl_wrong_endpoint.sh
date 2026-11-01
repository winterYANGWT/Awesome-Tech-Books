# 3.6 crictl 的 endpoint、inspect、stats、logs，以及节点上的 containerd
NODE=$(kind get nodes --name shorty)

# 第一种错法：路径写错，socket 根本不存在。
#> fails
#> expect: no such file or directory
docker exec "$NODE" crictl --runtime-endpoint unix:///run/containerd/containerd.sock.bak ps

# 第二种错法：路径存在，但不是 CRI socket，连接被拒绝。
#> fails
#> expect: connection refused
docker exec "$NODE" crictl --runtime-endpoint unix:///dev/null ps

# 指回节点的 containerd，恢复。默认 endpoint 也可以从 /etc/crictl.yaml 读。
docker exec "$NODE" crictl --runtime-endpoint unix:///run/containerd/containerd.sock ps | sed -n "1,4p"

# crictl info 是运行时的总览：默认 handler、已配置的 handler、runc 的 runtimeType。
docker exec "$NODE" crictl info | jq '{defaultRuntime: .config.containerd.defaultRuntimeName,
  handlers: (.config.containerd.runtimes | keys),
  runcType: .config.containerd.runtimes.runc.runtimeType}'

# 对一个业务容器做 inspect、stats、logs。ID 从 crictl ps 取，不用 Docker 的短 ID。
pod_id=$(docker exec "$NODE" crictl pods --namespace shorty -q)
cid=$(docker exec "$NODE" crictl ps --pod "$pod_id" -q)
docker exec "$NODE" crictl inspect "$cid" \
  | jq '{state: .status.state, exitCode: .status.exitCode}'
docker exec "$NODE" crictl stats "$cid"
docker exec "$NODE" crictl logs --tail=3 "$cid"

# 最后一道：节点上的 containerd 服务和它的日志。
docker exec "$NODE" systemctl status containerd --no-pager | sed -n "1,8p"
docker exec "$NODE" journalctl -u containerd -n 5 --no-pager
