# 24.2 先确认默认 CNI 是 kindnet，并且每个节点都有一个 kindnet Pod。
kubectl -n kube-system get daemonset kindnet
kubectl -n kube-system get pods -l app=kindnet -o wide

# 在 net-demo 里加一条只拒绝 Ingress 的策略；客户端的出站不受影响。
kubectl apply -f kindnet-deny-ingress.yaml
kubectl -n net-demo get networkpolicy

# 策略生效需要一点时间：反复请求，直到第一次被拒绝。
# 如果 kindnet 不执行 NetworkPolicy，这个循环会跑完并报错。
server_ip=$(kubectl -n net-demo get pod net-server -o jsonpath='{.status.podIP}')
denied=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n net-demo exec net-client -- wget -T 2 -qO- "http://$server_ip/" >/dev/null 2>&1; then
    sleep 1
  else
    echo "denied: kindnet 执行了 NetworkPolicy，wget 超时"
    denied=1
    break
  fi
done
if [ "$denied" -eq 0 ]; then
  echo "still allowed: NetworkPolicy 被忽略"
fi
[ "$denied" -eq 1 ]

# 策略删掉后网络恢复，说明刚才的拒绝来自这条策略，不是 Pod 自己挂了。
kubectl -n net-demo delete networkpolicy deny-ingress
restored=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n net-demo exec net-client -- wget -T 2 -qO- "http://$server_ip/" >/dev/null 2>&1; then
    echo "restored: 删除策略后 wget 恢复"
    restored=1
    break
  fi
  sleep 1
done
[ "$restored" -eq 1 ]

# 这组演示对象不留在集群里。
kubectl delete namespace net-demo --wait=true
