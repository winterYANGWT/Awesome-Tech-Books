#!/usr/bin/env bash
# shorty 端到端冒烟测试。在 lab 里执行：scripts/lab.sh exec 'bash src/code/app/smoke.sh'
set -euo pipefail
cd "$(dirname "$0")"

base=http://localhost:8080
compose=(docker compose)

cleanup() {
  "${compose[@]}" --profile tools down -v --remove-orphans >/dev/null 2>&1 || true
}
trap cleanup EXIT

echo "==> 清理旧环境并构建启动"
cleanup
"${compose[@]}" up -d --build

echo "==> 等待 api /readyz"
for i in $(seq 1 90); do
  if curl -fsS "$base/readyz" >/dev/null 2>&1; then
    break
  fi
  if [ "$i" = 90 ]; then
    echo "api 未就绪" >&2
    "${compose[@]}" logs api >&2
    exit 1
  fi
  sleep 1
done
curl -fsS "$base/readyz"

echo "==> 创建指向 http://api:8080/ 的短链"
created=$(curl -fsS -X POST "$base/api/links" \
  -H 'Content-Type: application/json' \
  -d '{"url":"http://api:8080/","ttl_seconds":3600}')
code=$(printf '%s' "$created" | jq -r .code)
short_url=$(printf '%s' "$created" | jq -r .short_url)
[ -n "$code" ] && [ "$code" != null ]
echo "code=$code short_url=$short_url"

echo "==> 检查 302 跳转和点击数"
location=$(curl -sS -D - -o /dev/null "$base/r/$code" | tr -d '\r' | awk 'tolower($1) == "location:" {print $2}')
[ "$location" = "http://api:8080/" ]
status=$(curl -sS -o /dev/null -w '%{http_code}' "$base/r/$code")
[ "$status" = 302 ]

echo "==> 等待 worker 写回标题"
title=
for i in $(seq 1 20); do
  detail=$(curl -fsS "$base/api/links/$code")
  title=$(printf '%s' "$detail" | jq -r '.title // empty')
  if [ "$title" = "Shorty" ]; then
    break
  fi
  sleep 1
done
[ "$title" = "Shorty" ] || {
  echo "title 未更新: $detail" >&2
  "${compose[@]}" logs worker >&2
  exit 1
}
clicks=$(printf '%s' "$detail" | jq -r .clicks)
[ "$clicks" -ge 2 ]
echo "title=$title clicks=$clicks"

echo "==> 检查 /metrics"
curl -fsS "$base/metrics" | grep -q 'shorty_http_requests_total'
curl -fsS "$base/metrics" | grep -q 'shorty_http_request_duration_seconds_bucket'
curl -fsS "$base/metrics" | grep -q 'shorty_http_in_flight_requests'
curl -fsS "$base/metrics" | grep -q 'shorty_process_start_time_seconds'

echo "==> 故障开关：延迟 500ms"
curl -fsS -X POST "$base/admin/fault?latency_ms=500" >/dev/null
elapsed=$(curl -sS -o /dev/null -w '%{time_total}' "$base/healthz")
awk -v t="$elapsed" 'BEGIN { exit !(t >= 0.45) }'

echo "==> 故障开关：not_ready"
curl -fsS -X POST "$base/admin/fault?not_ready=true" >/dev/null
status=$(curl -sS -o /dev/null -w '%{http_code}' "$base/readyz")
[ "$status" = 503 ]
curl -fsS -X POST "$base/admin/fault?latency_ms=0&not_ready=false" >/dev/null
curl -fsS "$base/admin/fault" | jq -e '.not_ready == false' >/dev/null

echo "==> docker compose stop api：检查优雅退出日志"
"${compose[@]}" stop api >/dev/null
logs=$("${compose[@]}" logs api 2>&1)
printf '%s\n' "$logs" | grep -q 'readiness 置为失败'
printf '%s\n' "$logs" | grep -q 'HTTP 服务已关闭'
printf '%s\n' "$logs" | grep -q 'shorty 退出'

echo "==> 全部通过，清理"
cleanup
trap - EXIT
