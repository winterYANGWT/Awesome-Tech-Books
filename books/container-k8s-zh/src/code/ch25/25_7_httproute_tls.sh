# 生成自签证书，CN 和 SAN 都是 shorty.local。
TLS_DIR=$(mktemp -d)
openssl req -x509 -newkey rsa:2048 -sha256 -nodes -days 2 \
  -keyout "$TLS_DIR/tls.key" -out "$TLS_DIR/tls.crt" \
  -subj '/CN=shorty.local' -addext 'subjectAltName=DNS:shorty.local' 2>/dev/null
kubectl -n shorty create secret tls shorty-tls \
  --cert="$TLS_DIR/tls.crt" --key="$TLS_DIR/tls.key"

# 加上 443 listener，TLS 在 Gateway 终止，后端仍然是明文 HTTP。
kubectl apply -f gateway-tls.yaml
kubectl -n shorty wait \
  --for=jsonpath='{.status.listeners[?(@.name=="https")].conditions[?(@.type=="Programmed")].status}'=True \
  gateway/shorty-gw --timeout=180s
# 加 listener 会滚动数据面 Pod；Available 可能还是旧 Pod，要等 rollout 完成。
kubectl -n shorty rollout status deployment/shorty-gw-nginx --timeout=180s
kubectl -n shorty wait --for=jsonpath='{.endpoints[0].addresses[0]}' endpointslice \
  -l kubernetes.io/service-name=shorty-gw-nginx --timeout=60s
kubectl -n shorty get svc shorty-gw-nginx

kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 18443:443 >/tmp/ch25-pf-tls.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -kfsS --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz >/dev/null 2>&1 && break
  sleep 0.2
done

# -k 跳过信任链校验；Host 头仍要匹配 HTTPRoute 的 hostnames。
curl -k -sS --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz

# 不加 -k 时，自签证书会被 curl 拒绝；这是预期行为，不是入口坏了。
#> fails
#> expect: self-signed certificate
curl -sS --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz

# 看服务端实际出示的证书：自签，subject 和 issuer 都是 shorty.local。
timeout 5 openssl s_client -connect 127.0.0.1:18443 -servername shorty.local </dev/null 2>/dev/null \
  | openssl x509 -noout -subject -issuer

# 删掉证书 Secret：listener 的 ResolvedRefs 变成 False，握手随即失败。
kubectl -n shorty delete secret shorty-tls
kubectl -n shorty wait \
  --for=jsonpath='{.status.listeners[?(@.name=="https")].conditions[?(@.type=="ResolvedRefs")].status}'=False \
  gateway/shorty-gw --timeout=120s
kubectl -n shorty get gateway shorty-gw \
  -o jsonpath='{range .status.listeners[*]}{.name}{" programmed="}{.conditions[?(@.type=="Programmed")].status}{" resolved="}{.conditions[?(@.type=="ResolvedRefs")].status}{"\n"}{end}'

#> fails
#> expect: curl: \(35\)
curl -k -sS --max-time 5 --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz

# 证书补回来后，NGF 重新加载配置，TLS 恢复。
kubectl -n shorty create secret tls shorty-tls \
  --cert="$TLS_DIR/tls.crt" --key="$TLS_DIR/tls.key"
kubectl -n shorty wait \
  --for=jsonpath='{.status.listeners[?(@.name=="https")].conditions[?(@.type=="ResolvedRefs")].status}'=True \
  gateway/shorty-gw --timeout=120s
kubectl -n shorty wait \
  --for=jsonpath='{.status.listeners[?(@.name=="https")].conditions[?(@.type=="Programmed")].status}'=True \
  gateway/shorty-gw --timeout=120s
kubectl -n shorty rollout status deployment/shorty-gw-nginx --timeout=180s
kubectl -n shorty wait --for=jsonpath='{.endpoints[0].addresses[0]}' endpointslice \
  -l kubernetes.io/service-name=shorty-gw-nginx --timeout=60s

# 证书重建后数据面可能换了一个 Pod，原来的 port-forward 会断开；重新建一条。
kill "$PF" 2>/dev/null || true
kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 18443:443 >/tmp/ch25-pf-tls2.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -kfsS --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz >/dev/null 2>&1 && break
  sleep 0.2
done
curl -k -sS --resolve shorty.local:18443:127.0.0.1 -H 'Host: shorty.local' https://shorty.local:18443/healthz

kill "$PF"
wait "$PF" 2>/dev/null || true
rm -rf "$TLS_DIR"
