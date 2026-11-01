# 24.1 两个 Pod 分别落在控制面和 worker，直接看对方的 Pod IP。
kubectl apply -f net-demo.yaml
kubectl -n net-demo wait --for=condition=Ready pod/net-server pod/net-client --timeout=120s

# NODE 列显示两个 Pod 在不在同一个节点；IP 列是 CNI 分配的 Pod IP。
kubectl -n net-demo get pods -o wide

# 在 worker 上的客户端 ping 控制面上的 nginx Pod。
server_ip=$(kubectl -n net-demo get pod net-server -o jsonpath='{.status.podIP}')
printf 'net-server IP: %s\n' "$server_ip"
kubectl -n net-demo exec net-client -- ping -c 2 "$server_ip"

# 同一个 IP 上取 HTTP，确认不是只有 ICMP 能过。
kubectl -n net-demo exec net-client -- wget -qO- "http://$server_ip/" | grep -o '<title>.*</title>'
