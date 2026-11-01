# 24.6 用一个独立的 api-canary 复现“目标标签写错”：它和 api 一样带 name=api，
# 但改标签不会触发 Deployment 滚动，便于观察策略是否还选中它。
kubectl -n shorty run api-canary --image=shorty:0.1 --restart=Never \
  --labels=app.kubernetes.io/name=api --port=8080 -- serve
kubectl -n shorty wait --for=condition=Ready pod/api-canary --timeout=120s

# 在 ingress 命名空间放一个测试客户端；namespace 自带 kubernetes.io/metadata.name 标签。
kubectl create namespace ingress
kubectl -n ingress run gateway-probe --image=busybox:1.37 --restart=Never \
  --labels=app.kubernetes.io/name=gateway -- sleep 3600
kubectl -n ingress wait --for=condition=Ready pod/gateway-probe --timeout=60s

# 先看策略本身：podSelector 选 api，from 里的 namespaceSelector 选 ingress。
kubectl -n shorty get networkpolicy allow-gateway -o yaml | sed -n '/^spec:/,$p'

canary_ip=$(kubectl -n shorty get pod api-canary -o jsonpath='{.status.podIP}')
printf 'api-canary IP: %s\n' "$canary_ip"

# 标签匹配时，ingress 里的客户端能访问 api-canary:8080。
allowed=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n ingress exec gateway-probe -- \
    wget -T 2 -qO- "http://$canary_ip:8080/readyz" >/dev/null 2>&1; then
    echo "allowed: allow-gateway 选中了 api-canary"
    allowed=1
    break
  fi
  sleep 1
done
[ "$allowed" -eq 1 ]

# 把目标 Pod 的 name 标签改掉：default-deny 仍然选中它，但 allow-gateway 不再匹配。
kubectl -n shorty label pod api-canary app.kubernetes.io/name=api-old --overwrite
denied=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n ingress exec gateway-probe -- \
    wget -T 2 -qO- "http://$canary_ip:8080/readyz" >/dev/null 2>&1; then
    sleep 1
  else
    echo "denied: 标签不再匹配 allow-gateway"
    denied=1
    break
  fi
done
[ "$denied" -eq 1 ]

# 改回来，策略重新匹配，访问恢复。
kubectl -n shorty label pod api-canary app.kubernetes.io/name=api --overwrite
restored=0
for i in $(seq 1 20); do
  if timeout 5 kubectl -n ingress exec gateway-probe -- \
    wget -T 2 -qO- "http://$canary_ip:8080/readyz" >/dev/null 2>&1; then
    echo "restored: 标签改回后再次放行"
    restored=1
    break
  fi
  sleep 1
done
[ "$restored" -eq 1 ]
kubectl -n shorty get pod api-canary --show-labels

# 本章的测试对象清掉；NetworkPolicy 留在 shorty。
kubectl -n shorty delete pod api-canary --wait=true
kubectl delete namespace ingress --wait=true
kubectl delete namespace probe --wait=true
