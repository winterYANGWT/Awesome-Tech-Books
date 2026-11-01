# 35.7 写一条 5xx 告警规则，制造 5xx，在 Prometheus 里看它从 inactive 到 firing。
# lab 不装 Alertmanager，所以告警只停在 Prometheus 里，不会真的发出去。

kubectl apply -f prometheusrule-shorty.yaml
kubectl -n shorty get prometheusrule

kubectl -n monitoring port-forward svc/kps-kube-prometheus-stack-prometheus 19090:9090 \
  >/tmp/ch35-pf-alert.log 2>&1 &
PF=$!
for i in $(seq 1 60); do
  curl -sf localhost:19090/-/ready >/dev/null 2>&1 && break
  sleep 0.5
done

# 规则要等 Prometheus 重载配置并完成第一次判定，state 才会从 unknown 变成 inactive。
for i in $(seq 1 120); do
  STATE=$(curl -s localhost:19090/api/v1/rules \
    | jq -r '.data.groups[] | select(.name == "shorty.rules")
        | .rules[] | select(.name == "ShortyHigh5xx") | .state')
  case "$STATE" in inactive|pending|firing) break ;; esac
  sleep 1
done
curl -s localhost:19090/api/v1/rules \
  | jq -r '.data.groups[] | select(.name == "shorty.rules")
      | .rules[] | "\(.name) state=\(.state) health=\(.health)"'

# 让 api 持续返回 500，持续到告警判定出来。
# 和 35.5 一样，Pod 刚就绪时 port-forward 偶尔会立刻退出，失败就重开。
POD=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')
: >/tmp/ch35-pf-load.log
FORWARDED=0
for attempt in 1 2 3; do
  kubectl -n shorty port-forward pod/$POD 18082:8080 >>/tmp/ch35-pf-load.log 2>&1 &
  PFAPI=$!
  for i in $(seq 1 40); do
    if curl -sf localhost:18082/healthz >/dev/null; then FORWARDED=1; break 2; fi
    sleep 0.5
  done
  kill $PFAPI 2>/dev/null || true
  sleep 1
done
if [ "$FORWARDED" -ne 1 ]; then
  cat /tmp/ch35-pf-load.log
  exit 1
fi

# 先制造一个 500，让 status=500 的序列以较小的值进入 Prometheus。
# 故障开关也作用于 /metrics：故障期间抓取会失败，Prometheus 可能只在故障结束后
# 才第一次看到这条序列，rate() 会把“从无到 27”算成 0。先留一个低值样本就不一样。
curl -s -o /dev/null -w 'prime error_rate=1 -> %{http_code}\n' \
  -X POST 'localhost:18082/admin/fault?error_rate=1'
curl -s -o /dev/null -w 'prime request -> %{http_code}\n' \
  -X POST localhost:18082/api/links \
  -H 'Content-Type: application/json' -d '{"url":"http://api:8080/"}'
curl -s -o /dev/null -w 'prime reset -> %{http_code}\n' \
  -X POST 'localhost:18082/admin/fault?error_rate=0'
for i in $(seq 1 60); do
  V=$(curl -s --get --data-urlencode 'query=sum(shorty_http_requests_total{status="500"})' \
    localhost:19090/api/v1/query | jq -r '.data.result[0].value[1] // "0"')
  [ "$V" != "0" ] && break
  sleep 1
done
echo "primed 5xx counter: $V"

# 现在把故障开关打开到 0.5，开始制造 5xx。
curl -s -o /dev/null -w 'set error_rate=0.5 -> %{http_code}\n' \
  -X POST 'localhost:18082/admin/fault?error_rate=0.5'

# 前台跑 10 秒、每秒 5 个请求，顺便数一下有多少个请求真的拿到 500。
# error_rate 也作用于 /healthz 和 /readyz：0.5 不会让探针连续失败三次，
# Pod 保持 Ready，才会留在 EndpointSlice 里被 Prometheus 抓到。
N500=0
for i in $(seq 1 10); do
  for j in 1 2 3 4 5; do
    CODE=$(curl -s -o /dev/null -w '%{http_code}' -X POST localhost:18082/api/links \
      -H 'Content-Type: application/json' -d '{"url":"http://api:8080/"}')
    [ "$CODE" = 500 ] && N500=$((N500 + 1))
  done
  sleep 1
done
#> expect: load: [1-9][0-9]*/50 returned 500
echo "load: $N500/50 returned 500"

# 先关掉故障开关，别让 liveness 把容器重启、计数清零。
curl -s -o /dev/null -w 'set error_rate=0 -> %{http_code}\n' \
  -X POST 'localhost:18082/admin/fault?error_rate=0'

for i in $(seq 1 90); do
  STATE=$(curl -s localhost:19090/api/v1/rules \
    | jq -r '.data.groups[] | select(.name == "shorty.rules")
        | .rules[] | select(.name == "ShortyHigh5xx") | .state')
  [ "$STATE" = firing ] && break
  sleep 1
done

# 规则状态和 ALERTS 序列都能证明它已经在 firing；没有 Alertmanager 就不会再往下走。
#> expect: state=firing
curl -s localhost:19090/api/v1/rules \
  | jq -r '.data.groups[] | select(.name == "shorty.rules")
      | .rules[] | select(.name == "ShortyHigh5xx")
      | "\(.name) state=\(.state) health=\(.health) lastError=\(.lastError // "-")"'
#> expect: ShortyHigh5xx firing
curl -s 'localhost:19090/api/v1/query?query=ALERTS%7Balertname%3D%22ShortyHigh5xx%22%7D' \
  | jq -r '.data.result[] | "\(.metric.alertname) \(.metric.alertstate) \(.value[1])"'

kill $PFAPI $PF 2>/dev/null || true
