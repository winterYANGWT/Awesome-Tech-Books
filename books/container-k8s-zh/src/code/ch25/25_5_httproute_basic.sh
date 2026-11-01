# Gateway 声明监听 80，hostname 限制为 shorty.local，只接受同 namespace 的 Route。
kubectl apply -f gateway.yaml

# NGF 会为这个 Gateway 创建数据面 Deployment 和 Service；等它把配置下发完。
kubectl -n shorty wait --for=condition=Programmed gateway/shorty-gw --timeout=180s
kubectl -n shorty get gateway shorty-gw
# Gateway 的 Programmed 只说明配置下发了；数据面 Pod 还要等镜像和探针。
kubectl -n shorty wait --for=condition=Available deployment \
  -l gateway.networking.k8s.io/gateway-name=shorty-gw --timeout=180s
kubectl -n shorty wait --for=jsonpath='{.endpoints[0].addresses[0]}' endpointslice \
  -l kubernetes.io/service-name=shorty-gw-nginx --timeout=60s
kubectl -n shorty get deploy,svc -l gateway.networking.k8s.io/gateway-name=shorty-gw

# HTTPRoute 只匹配 /healthz，并绑定到 Gateway 的 http listener。
kubectl apply -f httproute-basic.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="Accepted")].status}'=True \
  httproute/shorty --timeout=60s
kubectl -n shorty get httproute shorty

# 把数据面的 80 端口转到本机 18080，再从 lab 发请求。
kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-basic.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/healthz >/dev/null 2>&1 && break
  sleep 0.2
done

# Host 匹配 shorty.local，路径命中 /healthz，落到 api 的 8080。
curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/healthz

kill "$PF"
wait "$PF" 2>/dev/null || true
