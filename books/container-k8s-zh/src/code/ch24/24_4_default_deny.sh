# 24.4 先在 probe 命名空间放一个客户端；它不在 shorty 里，
# 所以 shorty 的默认拒绝只会挡住到 api 的入站，不会影响它自己的出站。
kubectl create namespace probe
kubectl -n probe run probe-client --image=busybox:1.37 --restart=Never -- sleep 3600
kubectl -n probe wait --for=condition=Ready pod/probe-client --timeout=60s

# 没有策略时，直接用 Pod IP 访问 api 的 /readyz，返回 ok。
api_ip=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{.items[0].status.podIP}')
printf 'api Pod IP: %s\n' "$api_ip"
kubectl -n probe exec probe-client -- wget -qO- "http://$api_ip:8080/readyz"

# 建一条选中所有 Pod、Ingress 和 Egress 都拒绝的策略。
kubectl apply -f default-deny.yaml
kubectl -n shorty get networkpolicy
kubectl -n shorty get networkpolicy default-deny -o yaml | sed -n '/^spec:/,$p'

# 同样用 Pod IP，不再依赖 DNS；等到第一次超时为止。
denied=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n probe exec probe-client -- wget -T 2 -qO- "http://$api_ip:8080/readyz" >/dev/null 2>&1; then
    sleep 1
  else
    echo "denied: 默认拒绝后 wget 超时"
    denied=1
    break
  fi
done
if [ "$denied" -eq 0 ]; then
  echo "still allowed: 策略没有生效"
fi
[ "$denied" -eq 1 ]
