# 35.3 让 Prometheus 抓 shorty 的 /metrics，并看 target 从 DOWN 到 UP。

# ServiceMonitor 通过 Service 的标签和端口名找 target，先把这两个对上。
kubectl -n shorty get svc api \
  -o jsonpath='{.spec.selector}{"\n"}{range .spec.ports[*]}{.name}: {.port} -> {.targetPort}{"\n"}{end}'

kubectl apply -f servicemonitor-shorty.yaml
kubectl -n shorty get servicemonitor

# 打开 Prometheus 的查询接口。
kubectl -n monitoring port-forward svc/kps-kube-prometheus-stack-prometheus 19090:9090 \
  >/tmp/ch35-pf-prom.log 2>&1 &
PF=$!
for i in $(seq 1 60); do
  curl -sf localhost:19090/-/ready >/dev/null 2>&1 && break
  sleep 0.5
done
curl -s localhost:19090/-/ready

# 等 target 出现并失败。shorty 的 default-deny 会丢掉 monitoring 发来的抓取请求。
for i in $(seq 1 180); do
  H=$(curl -s localhost:19090/api/v1/targets \
    | jq -r '.data.activeTargets[] | select(.labels.namespace=="shorty") | .health' | head -1)
  [ "$H" = down ] && break
  sleep 1
done
#> expect: health=down
curl -s localhost:19090/api/v1/targets \
  | jq -r '.data.activeTargets[] | select(.labels.namespace=="shorty")
      | "\(.scrapeUrl) health=\(.health) \(.lastError // "-")"'

# target 的地址是 Pod IP:8080，不是 Service IP：Prometheus 直接抓端点。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
  -o jsonpath='{range .items[*].endpoints[*]}{.addresses[0]}{" ready="}{.conditions.ready}{"\n"}{end}'

# 放行 monitoring 的入站抓取，再等下一次 scrape。
kubectl apply -f allow-prometheus-networkpolicy.yaml
for i in $(seq 1 180); do
  H=$(curl -s localhost:19090/api/v1/targets \
    | jq -r '.data.activeTargets[] | select(.labels.namespace=="shorty") | .health' | head -1)
  [ "$H" = up ] && break
  sleep 1
done
#> expect: health=up
curl -s localhost:19090/api/v1/targets \
  | jq -r '.data.activeTargets[] | select(.labels.namespace=="shorty")
      | "\(.scrapeUrl) health=\(.health) \(.lastError // "-")"'

# target UP 之后，shorty 的请求计数就能在 Prometheus 里查到了。
#> head: 8
curl -s 'localhost:19090/api/v1/query?query=shorty_http_requests_total' \
  | jq -r '.data.result[] | "\(.metric.route) \(.metric.status) \(.value[1])"'
kill $PF
