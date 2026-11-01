# 14.6：watch 需要容器里有 stat/mkdir/rmdir，且运行用户能写目标目录
# 演示页面由 page/Dockerfile 构建。
#> tail: 8
#> timeout: 120
docker compose -f compose.yaml -f compose.watch.yaml --profile watch up -d page
for i in $(seq 1 30); do
  curl -fsS localhost:8081/ >/dev/null 2>&1 && break
  sleep 0.5
done
curl -fsS localhost:8081/ | grep -o '<title>.*</title>'
curl -fsS localhost:8081/VERSION

# sync：只改 index.html，watch 把文件复制进容器，不重建镜像
rm -f /tmp/ch14-watch-sync.log
setsid docker compose -f compose.yaml -f compose.watch.yaml --profile watch watch page > /tmp/ch14-watch-sync.log 2>&1 &
wpid=$!
for i in $(seq 1 40); do
  grep -q 'Watch enabled' /tmp/ch14-watch-sync.log && break
  sleep 0.5
done
sed -i 's/Shorty page v1/Shorty page v2/g' page/index.html
for i in $(seq 1 40); do
  curl -fsS localhost:8081/ 2>/dev/null | grep -q 'Shorty page v2' && break
  sleep 0.5
done
kill -- -"$wpid" 2>/dev/null || true
wait "$wpid" 2>/dev/null || true
grep -E 'Syncing service' /tmp/ch14-watch-sync.log | tail -n 8
#> expect: Shorty page v2
curl -fsS localhost:8081/ | grep -o '<title>.*</title>'

# rebuild：改 VERSION，Compose 重新构建镜像并替换容器
rm -f /tmp/ch14-watch-rebuild.log
setsid docker compose -f compose.yaml -f compose.watch.yaml --profile watch watch page > /tmp/ch14-watch-rebuild.log 2>&1 &
wpid=$!
for i in $(seq 1 40); do
  grep -q 'Watch enabled' /tmp/ch14-watch-rebuild.log && break
  sleep 0.5
done
printf 'v2\n' > page/VERSION
for i in $(seq 1 60); do
  curl -fsS localhost:8081/VERSION 2>/dev/null | grep -qx 'v2' && break
  sleep 0.5
done
kill -- -"$wpid" 2>/dev/null || true
wait "$wpid" 2>/dev/null || true
grep -E 'Rebuilding service|Recreated' /tmp/ch14-watch-rebuild.log | sort -u
#> expect: ^v2$
curl -fsS localhost:8081/VERSION

# 还原示例文件并移除 page，核心的五个服务不动
printf 'v1\n' > page/VERSION
sed -i 's/Shorty page v2/Shorty page v1/g' page/index.html
docker compose -f compose.yaml -f compose.watch.yaml --profile watch rm -sf page
