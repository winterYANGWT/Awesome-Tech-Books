# 35.4 shorty 的 JSON 日志：用 jq 过滤，并在崩溃后用 --previous 看上一轮。

# 固定看一个 api Pod 的日志，端口也转发到它。
POD=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')
kubectl -n shorty port-forward pod/$POD 18082:8080 >/tmp/ch35-pf-logs.log 2>&1 &
PF=$!
for i in $(seq 1 60); do
  curl -sf localhost:18082/healthz >/dev/null 2>&1 && break
  sleep 0.2
done

# 制造三种结果：200、404、500。
curl -s -o /dev/null -w 'GET / -> %{http_code}\n' localhost:18082/
curl -s -o /dev/null -w 'GET /r/nope -> %{http_code}\n' localhost:18082/r/nope
curl -s -X POST 'localhost:18082/admin/fault?error_rate=1' >/dev/null
for i in 1 2 3; do
  curl -s -o /dev/null -w 'POST /api/links -> %{http_code}\n' -X POST \
    localhost:18082/api/links -H 'Content-Type: application/json' \
    -d '{"url":"http://api:8080/","ttl_seconds":60}'
done
curl -s -X POST 'localhost:18082/admin/fault?error_rate=0' >/dev/null

# 一行一个 JSON 对象；按 msg 和 status 过滤比 grep 文本可靠。
kubectl -n shorty logs $POD --tail=60 \
  | jq -c 'select(.msg == "request" and .status >= 400) | {status, path, duration_ms}'

# 日志落在节点上：/var/log/pods 是真实文件，/var/log/containers 是指向它的符号链接。
NODE=$(kubectl -n shorty get pod $POD -o jsonpath='{.spec.nodeName}')
docker exec "$NODE" ls -l /var/log/pods | grep "$POD"
docker exec "$NODE" ls -l /var/log/containers | grep "$POD"

# kubelet 只保留最新的日志文件；容器重启后，上一轮要加 --previous。
curl -s -X POST 'localhost:18082/admin/fault?crash_after=3' >/dev/null
kubectl -n shorty wait \
  --for=jsonpath='{.status.containerStatuses[0].restartCount}'=1 pod/$POD --timeout=180s
kubectl -n shorty get pod $POD -o jsonpath='exitCode={.status.containerStatuses[0].lastState.terminated.exitCode} reason={.status.containerStatuses[0].lastState.terminated.reason}{"\n"}'
#> expect: 故障开关触发退出
kubectl -n shorty logs $POD --previous --tail=4
kill $PF 2>/dev/null || true
