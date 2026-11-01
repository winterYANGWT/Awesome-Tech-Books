# 加一个静态前端：ConfigMap 提供 index.html，nginx 作为 / 的后端。
kubectl apply -f web.yaml
kubectl -n shorty rollout status deployment/web --timeout=120s

# /api 给 api，/ 给 web；两条规则在同一个 HTTPRoute 里。
kubectl apply -f httproute-path.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s
kubectl -n shorty get httproute shorty

kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-path.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/ >/dev/null 2>&1 && break
  sleep 0.2
done

# / 命中前端，返回 ConfigMap 里的标题。
curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/ | grep -o '<title>.*</title>'

# /api 命中 shorty 的 API；先创建一条短链，再读它。
code=$(curl -fsS -X POST -H 'Host: shorty.local' -H 'Content-Type: application/json' \
  -d '{"url":"http://api:8080/","ttl_seconds":3600}' \
  http://127.0.0.1:18080/api/links | sed -n 's/.*"code":"\([^"]*\)".*/\1/p')
test -n "$code" && echo "link created"
curl -sS -H 'Host: shorty.local' "http://127.0.0.1:18080/api/links/$code" \
  | grep -o '"url":"[^"]*"'

kill "$PF"
wait "$PF" 2>/dev/null || true
