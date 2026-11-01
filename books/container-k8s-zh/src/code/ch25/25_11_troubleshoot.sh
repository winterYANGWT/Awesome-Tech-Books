# 先看 Gateway 和 HTTPRoute 的整体状态；ACCEPTED 和 RESOLVEDREFS 是两列关键信息。
kubectl -n shorty get gateway,httproute

# listener 的 conditions 说明端口和证书引用是否被实现接受。
kubectl -n shorty get gateway shorty-gw \
  -o jsonpath='{range .status.listeners[*]}{.name}{" programmed="}{.conditions[?(@.type=="Programmed")].status}{" resolved="}{.conditions[?(@.type=="ResolvedRefs")].status}{"\n"}{end}'

# 后端端口以 Service 为准；HTTPRoute 里写错端口，ResolvedRefs 会变成 False。
kubectl -n shorty get svc api -o jsonpath='{.spec.ports[*].port}{"\n"}'
kubectl apply -f httproute-bad-port.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.parents[0].conditions[?(@.type=="ResolvedRefs")].status}'=False \
  httproute/shorty-bad-port --timeout=60s
kubectl -n shorty get httproute shorty-bad-port \
  -o jsonpath='{range .status.parents[*]}{.parentRef.name}{" accepted="}{.conditions[?(@.type=="Accepted")].status}{" resolved="}{.conditions[?(@.type=="ResolvedRefs")].status}{" reason="}{.conditions[?(@.type=="ResolvedRefs")].reason}{"\n"}{end}'
kubectl -n shorty delete httproute shorty-bad-port

kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-trouble.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/healthz >/dev/null 2>&1 && break
  sleep 0.2
done

# Host 不在任何 HTTPRoute 的 hostnames 里：数据面返回 404。
curl -sS -o /dev/null -w '%{http_code}\n' -H 'Host: other.local' http://127.0.0.1:18080/healthz

# 同一个地址换成 shorty.local 就是 200。
curl -sS -o /dev/null -w '%{http_code}\n' -H 'Host: shorty.local' http://127.0.0.1:18080/healthz

# 控制器日志只摘 msg 字段，避免每行的时间戳和版本号干扰阅读。
kubectl -n nginx-gateway logs deploy/ngf-nginx-gateway-fabric --tail=20 \
  | grep -o '"msg":"[^"]*"' | tail -6 || true

kill "$PF"
wait "$PF" 2>/dev/null || true
