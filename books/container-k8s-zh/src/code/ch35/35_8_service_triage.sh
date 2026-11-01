# 35.8 排障：Pod 是 Running、Ready，Gateway 却返回 502。
# 按 EndpointSlice -> readiness -> Pod 端口 -> NetworkPolicy -> 入口 的顺序查。

kubectl -n shorty get pods -l app.kubernetes.io/name=api
kubectl -n shorty get svc api

# 复现：把 Service 的 selector 改成一个不存在的标签值。
kubectl -n shorty patch svc api \
  -p '{"spec":{"selector":{"app.kubernetes.io/name":"api-typo"}}}'

# 等 EndpointSlice 控制器把地址摘干净，再打 Gateway。
for i in $(seq 1 60); do
  N=$(kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
    -o jsonpath='{.items[*].endpoints[*].addresses[*]}' | wc -w | tr -d ' ')
  [ "$N" -eq 0 ] && break
  sleep 1
done
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api
kubectl -n shorty port-forward svc/shorty-gw-nginx 18081:80 >/tmp/ch35-pf-502.log 2>&1 &
PFG=$!
for i in $(seq 1 60); do
  curl -s -o /dev/null localhost:18081/ && break
  sleep 0.2
done

# 等 NGF 把新的 EndpointSlice 下发到数据面，再确认 5xx。
for i in $(seq 1 60); do
  CODE=$(curl -s -o /dev/null -w '%{http_code}' localhost:18081/healthz)
  case "$CODE" in 50*) break ;; esac
  sleep 1
done
#> expect: 50[23]
echo "gateway /healthz -> $CODE"

# 第 1 步：EndpointSlice 没有地址，说明 Service 这一层就没有后端。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
  -o jsonpath='{range .items[*]}addresses={.endpoints[*].addresses}{" ready="}{.endpoints[*].conditions.ready}{"\n"}{end}'

# 第 2 步：Pod 本身是 Ready 的，端口 8080 也在监听，排除 readiness 和端口。
kubectl -n shorty get pods -l app.kubernetes.io/name=api
POD=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')
kubectl -n shorty get pod $POD \
  -o jsonpath='{range .spec.containers[*]}{.name} port={.ports[0].containerPort}{"\n"}{end}'

# 第 3 步：绕过 Service，直接对 Pod 发请求，应用是好的。
kubectl -n shorty port-forward pod/$POD 18082:8080 >/tmp/ch35-pf-app.log 2>&1 &
PF=$!
for i in $(seq 1 60); do
  curl -sf localhost:18082/healthz >/dev/null 2>&1 && break
  sleep 0.2
done
curl -sS localhost:18082/healthz
kubectl -n shorty top pod $POD

# 第 4 步：回来查 Service 的 selector；它已经选不到任何 Pod。
kubectl -n shorty get svc api -o jsonpath='selector={.spec.selector}{"\n"}'
kubectl -n shorty get pods -l app.kubernetes.io/name=api-typo

# 改回正确的 selector，EndpointSlice 会重新出现地址。
kubectl -n shorty patch svc api \
  -p '{"spec":{"selector":{"app.kubernetes.io/name":"api"}}}'
for i in $(seq 1 60); do
  N=$(kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
    -o jsonpath='{.items[*].endpoints[*].addresses[*]}' | wc -w | tr -d ' ')
  [ "$N" -ge 1 ] && break
  sleep 1
done
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api
for i in $(seq 1 60); do
  CODE=$(curl -s -o /dev/null -w '%{http_code}' localhost:18081/healthz)
  [ "$CODE" = 200 ] && break
  sleep 1
done
echo "gateway /healthz -> $CODE"
kill $PF $PFG
