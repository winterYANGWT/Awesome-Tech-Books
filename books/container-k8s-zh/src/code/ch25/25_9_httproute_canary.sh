# web-v2 已经由上一个示例部署；这里改用 header 做确定性路由。
# 带 X-Canary: true 的请求走 web-v2，其余走 web，两条规则都去掉 /canary 前缀。
kubectl apply -f httproute-canary.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s
kubectl -n shorty get httproute

kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-canary.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/canary >/dev/null 2>&1 && break
  sleep 0.2
done

# 等数据面加载新规则：上一个示例把权重切到了 100/0，这里等到默认请求回到 v1。
for i in $(seq 1 50); do
  t=$(curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | sed -n 's:.*<title>\(.*\)</title>.*:\1:p')
  [ "$t" = "Shorty frontend" ] && break
  sleep 0.2
done

# 默认规则：不带 header，落在 v1。
curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | grep -o '<title>.*</title>'

# header 规则：落在 v2。
curl -sS -H 'Host: shorty.local' -H 'X-Canary: true' \
  http://127.0.0.1:18080/canary | grep -o '<title>.*</title>'

kill "$PF"
wait "$PF" 2>/dev/null || true

# 恢复主路由，并清掉灰度用的对象。
kubectl apply -f httproute-path.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s
kubectl delete -f web-v2.yaml
kubectl -n shorty get httproute
