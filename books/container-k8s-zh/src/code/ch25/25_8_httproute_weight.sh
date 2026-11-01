# 第二个前端版本，标题不同，用来区分流量落到了哪边。
kubectl apply -f web-v2.yaml
kubectl -n shorty rollout status deployment/web-v2 --timeout=120s

# 一条 /canary 规则里放两个 backendRef：web 90、web-v2 10。
kubectl apply -f httproute-weight-90-10.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s
kubectl -n shorty get httproute

kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-weight.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/canary >/dev/null 2>&1 && break
  sleep 0.2
done

# 等数据面加载新规则：旧路由下 /canary 全部落到 web，不会出现 v2。
for i in $(seq 1 50); do
  t=$(curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | sed -n 's:.*<title>\(.*\)</title>.*:\1:p')
  [ "$t" = "Shorty frontend v2" ] && break
  sleep 0.2
done

# 发 100 次请求，按返回的标题计数。
v1=0; v2=0
for i in $(seq 1 100); do
  t=$(curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | sed -n 's:.*<title>\(.*\)</title>.*:\1:p')
  if [ "$t" = "Shorty frontend v2" ]; then v2=$((v2+1))
  elif [ "$t" = "Shorty frontend" ]; then v1=$((v1+1)); fi
done
#> expect: v1=[1-9][0-9]* v2=[1-9][0-9]* total=100
printf 'v1=%d v2=%d total=%d\n' "$v1" "$v2" "$((v1+v2))"

# 把 web 的权重降到 0、web-v2 升到 100，再发 20 次，全部落到 v2。
kubectl apply -f httproute-weight-0-100.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s

# 等配置 reload：连续 10 次都是 v2 才认为新权重已经生效。
ok=0
for i in $(seq 1 100); do
  t=$(curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | sed -n 's:.*<title>\(.*\)</title>.*:\1:p')
  if [ "$t" = "Shorty frontend v2" ]; then ok=$((ok+1)); else ok=0; fi
  [ "$ok" -ge 10 ] && break
  sleep 0.1
done

v2=0
for i in $(seq 1 20); do
  t=$(curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/canary | sed -n 's:.*<title>\(.*\)</title>.*:\1:p')
  [ "$t" = "Shorty frontend v2" ] && v2=$((v2+1))
done
#> expect: v2=20/20
echo "v2=$v2/20"

kill "$PF"
wait "$PF" 2>/dev/null || true
