# 35.5 部署 OTel Collector，收一条 service.name=shorty 的 OTLP span，从 Collector 日志里看到。
# shorty 源码里没有 OTel SDK，这里用它接入 SDK 后会发出的同一条 OTLP 请求代替。

kubectl apply -f otel-collector.yaml
kubectl -n monitoring rollout status deployment/otel-collector --timeout=180s
kubectl -n monitoring get deploy,svc otel-collector

# 把 OTLP/HTTP 端口 4318 转发到本机；应用侧的 exporter 也就是往这个地址发。
# 直接转发到 Pod，避免 Service 的 EndpointSlice 还没更新时 port-forward 找不到后端。
# Pod 刚 Running 时 port-forward 偶尔会立刻退出，失败就重开一次。
POD=$(kubectl -n monitoring get pod -l app=otel-collector \
  -o jsonpath='{.items[0].metadata.name}')
: >/tmp/ch35-pf-otel.log
FORWARDED=0
for attempt in 1 2 3; do
  kubectl -n monitoring port-forward pod/$POD 14318:4318 >>/tmp/ch35-pf-otel.log 2>&1 &
  PF=$!
  for i in $(seq 1 40); do
    if curl -s -o /dev/null localhost:14318/; then FORWARDED=1; break 2; fi
    sleep 0.5
  done
  kill $PF 2>/dev/null || true
  sleep 1
done
#> hide
if [ "$FORWARDED" -ne 1 ]; then
  echo "=== port-forward log ==="
  cat /tmp/ch35-pf-otel.log
  echo "=== pods ==="
  kubectl -n monitoring get pods -o wide
  echo "=== collector log ==="
  kubectl -n monitoring logs deploy/otel-collector --tail=20
  exit 1
fi

# OTLP/JSON 是 OTLP 的一种编码，Collector 的 otlp receiver 默认两种都收。
curl -sS -X POST localhost:14318/v1/traces \
  -H 'Content-Type: application/json' --data-binary @otel-span.json
echo

# debug exporter 把 span 打到 Collector 的 stdout：先等它出现，再读日志。
for i in $(seq 1 30); do
  kubectl -n monitoring logs deploy/otel-collector --tail=60 | grep -q 'POST /api/links' && break
  sleep 1
done
#> expect: service.name.*shorty
kubectl -n monitoring logs deploy/otel-collector --tail=28
kill $PF
