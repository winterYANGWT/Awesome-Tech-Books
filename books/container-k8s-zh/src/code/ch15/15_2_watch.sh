rm -rf /tmp/ch15-watch
mkdir -p /tmp/ch15-watch/html
cat > /tmp/ch15-watch/compose.yaml <<'YAML'
services:
  web:
    build: .
    image: ch15-watch-web
    ports:
      - "18081:80"
    develop:
      watch:
        - action: sync
          path: ./html
          target: /usr/share/nginx/html
          ignore:
            - "*.tmp"
YAML
cat > /tmp/ch15-watch/Dockerfile <<'DOCKER'
FROM nginx:1.29-alpine
COPY html/ /usr/share/nginx/html/
DOCKER
cat > /tmp/ch15-watch/html/index.html <<'HTML'
<!doctype html>
<html lang="zh-CN">
  <head><meta charset="utf-8"><title>Shorty watch</title></head>
  <body><h1>Version 1</h1></body>
</html>
HTML
cd /tmp/ch15-watch
#> tail: 12
docker compose up -d --build

# watch 只同步 index.html；*.tmp 按 ignore 规则跳过。
(
  set -e
  docker compose watch --no-up > watch.log 2>&1 &
  watch_pid=$!
  trap 'kill $watch_pid 2>/dev/null || true' EXIT
  for _ in $(seq 1 60); do
    grep -q 'Watch enabled' watch.log && break
    sleep 1
  done
  grep -q 'Watch enabled' watch.log
  echo 'watch: enabled'
  echo ignored > html/ignored.tmp
  sed -i 's/Version 1/Version 2/' html/index.html
  for _ in $(seq 1 60); do
    curl -fsS http://localhost:18081/ 2>/dev/null | grep -q 'Version 2' && break
    sleep 1
  done
  curl -fsS http://localhost:18081/ | grep -o 'Version 2'
  docker compose exec -T web ls /usr/share/nginx/html
  kill "$watch_pid"
  wait "$watch_pid" 2>/dev/null || true
  trap - EXIT
  tail -n 10 watch.log
)

docker compose down
