# 34.4 金丝雀：两个 Deployment、两个 Service，HTTPRoute 按权重分流。
kubectl apply -f api-canary.yaml
kubectl -n shorty rollout status deploy/api-canary --timeout=180s
kubectl apply -f canary-route.yaml
kubectl -n shorty get httproute shorty \
    -o jsonpath='{range .spec.rules[0].backendRefs[*]}{.name}={.weight}{" "}{end}{"\n"}'

# 通过 Gateway 的 Service 发请求。port-forward 起在 lab 主机上，不经过集群 DNS。
kubectl -n shorty port-forward svc/shorty-gw-nginx 18080:80 >/tmp/ch34-gw-pf.log 2>&1 &
GW_PF=$!
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://127.0.0.1:18080/healthz >/dev/null

# 200 个创建请求，按 short_url 里的 base URL 统计落到了哪一边。
stable=0
canary=0
for _ in $(seq 1 200); do
    body=$(curl -sS --max-time 3 -X POST http://127.0.0.1:18080/api/links \
        -H 'Content-Type: application/json' \
        -d '{"url":"http://api:8080/","ttl_seconds":60}')
    if printf '%s' "$body" | grep -q 'canary.shorty'; then
        canary=$((canary + 1))
    else
        stable=$((stable + 1))
    fi
done
printf 'stable=%d canary=%d total=%d\n' "$stable" "$canary" "$((stable + canary))"
[ "$canary" -ge 5 ] && [ "$canary" -le 50 ] && echo 'ratio ok'

# 给金丝雀打开 100% 错误率，但不要碰健康探针所在的那个进程参数——
# 这里用的是 /admin/fault，它会连 /healthz 一起返回 500，所以只留很短的时间窗。
kubectl apply -f canary-fault-pod.yaml
kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Succeeded pod/canary-fault --timeout=60s
kubectl -n shorty logs canary-fault

# 立刻发 100 个请求；金丝雀大约占 10%，应该能看到 5xx。
fivexx=0
ok=0
for _ in $(seq 1 100); do
    code=$(curl -sS --max-time 3 -o /dev/null -w '%{http_code}' \
        -X POST http://127.0.0.1:18080/api/links \
        -H 'Content-Type: application/json' \
        -d '{"url":"http://api:8080/","ttl_seconds":60}')
    if [ "$code" -ge 500 ]; then
        fivexx=$((fivexx + 1))
    else
        ok=$((ok + 1))
    fi
done
printf '5xx=%d ok=%d\n' "$fivexx" "$ok"
[ "$fivexx" -gt 0 ] && echo 'canary is failing'

# 把权重调回 0。NGF 重载配置没有可直接 wait 的条件，轮询到连续 5 个 2xx 再统计。
kubectl apply -f canary-route-zero.yaml
kubectl -n shorty get httproute shorty \
    -o jsonpath='{range .spec.rules[0].backendRefs[*]}{.name}={.weight}{" "}{end}{"\n"}'
consecutive=0
for _ in $(seq 1 60); do
    code=$(curl -sS --max-time 3 -o /dev/null -w '%{http_code}' \
        -X POST http://127.0.0.1:18080/api/links \
        -H 'Content-Type: application/json' \
        -d '{"url":"http://api:8080/","ttl_seconds":60}')
    if [ "$code" -ge 200 ] && [ "$code" -lt 300 ]; then
        consecutive=$((consecutive + 1))
    else
        consecutive=0
    fi
    [ "$consecutive" -ge 5 ] && break
    sleep 1
done
[ "$consecutive" -ge 5 ] && echo 'route reloaded'


fivexx=0
canary_hits=0
for _ in $(seq 1 100); do
    resp=$(curl -sS --max-time 3 -w '\n%{http_code}' \
        -X POST http://127.0.0.1:18080/api/links \
        -H 'Content-Type: application/json' \
        -d '{"url":"http://api:8080/","ttl_seconds":60}')
    code=${resp##*$'\n'}
    body=${resp%$'\n'*}
    if [ "$code" -ge 500 ]; then
        fivexx=$((fivexx + 1))
    fi
    if printf '%s' "$body" | grep -q 'canary.shorty'; then
        canary_hits=$((canary_hits + 1))
    fi
done
printf 'after rollback: 5xx=%d canary_hits=%d\n' "$fivexx" "$canary_hits"
[ "$fivexx" -eq 0 ] && [ "$canary_hits" -eq 0 ] && echo 'rollback ok'
# 演练结束：删掉带故障的 canary Pod，恢复健康后把权重调回 90/10，给下一章留状态。
kubectl -n shorty delete pod -l app.kubernetes.io/name=api-canary
kubectl -n shorty rollout status deploy/api-canary --timeout=120s
kubectl apply -f canary-route.yaml
restored=0
for _ in $(seq 1 60); do
    body=$(curl -sS --max-time 3 -X POST http://127.0.0.1:18080/api/links \
        -H 'Content-Type: application/json' \
        -d '{"url":"http://api:8080/","ttl_seconds":60}')
    if printf '%s' "$body" | grep -q 'canary.shorty'; then
        restored=1
        break
    fi
    sleep 1
done
printf 'canary_restored=%d\n' "$restored"
[ "$restored" -eq 1 ] && echo 'canary restored'
kill "$GW_PF" 2>/dev/null || true
