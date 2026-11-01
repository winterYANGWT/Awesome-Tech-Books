# 从集群里读 25.2 创建的 Ingress，转成 Gateway API 清单写到文件。
ingress2gateway print --providers=ingress-nginx -n shorty -o yaml > ingress2gateway.yaml

# 警告里点出两条 nginx 注解没有标准对应物；YAML 本身只保留 host/path/backend/TLS。
head -80 ingress2gateway.yaml

# 转换器按 Ingress 生成固定名字：Gateway 叫 nginx，HTTPRoute 叫 shorty-shorty-local。
# 把名字对齐到集群里已有的 shorty-gw / shorty，再和线上对象做 diff。
sed -e 's/^  name: nginx$/  name: shorty-gw/' \
    -e 's/^  - name: nginx$/  - name: shorty-gw/' \
    -e 's/^  name: shorty-shorty-local$/  name: shorty/' \
    -e 's/^  name: shorty-shorty-local-http$/  name: shorty-http-redirect/' \
    ingress2gateway.yaml > ingress2gateway-fixed.yaml

kubectl diff -f ingress2gateway-fixed.yaml > /tmp/ch25-diff.txt 2>&1; rc=$?
head -60 /tmp/ch25-diff.txt
echo "kubectl diff exit=$rc"

# 两条注解没有被搬到 HTTPRoute 上：body size 和 rate limit 要按实现自己的策略重写。
if grep -q 'nginx.ingress' ingress2gateway-fixed.yaml; then
  echo "annotations copied"
else
  echo "nginx annotations not copied"
fi

# 生成的清单能通过服务端 schema 校验；这里只 dry-run，不覆盖线上对象。
kubectl apply --dry-run=server -f ingress2gateway-fixed.yaml

# 切流完成后删掉旧 Ingress；清单留在 Git 里就是回滚路径。
kubectl -n shorty get ingress shorty -o yaml > /tmp/ch25-ingress-backup.yaml
kubectl -n shorty delete ingress shorty
kubectl -n shorty get ingress

# 旧 Ingress 删掉后，Gateway 这条路仍然可用。
kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch25-pf-i2g.log 2>&1 & PF=$!
for i in $(seq 1 50); do
  curl -fsS -H 'Host: shorty.local' http://127.0.0.1:18080/healthz >/dev/null 2>&1 && break
  sleep 0.2
done
curl -sS -H 'Host: shorty.local' http://127.0.0.1:18080/healthz

kill "$PF"
wait "$PF" 2>/dev/null || true

# 转换文件是这次演示的中间产物，清理掉，下次运行重新生成。
rm -f ingress2gateway.yaml ingress2gateway-fixed.yaml
