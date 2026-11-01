# 先确保没有旧 builder，第一次构建会从零开始。
docker buildx rm -f ch15ci >/dev/null 2>&1 || true
bash ci demo-1 > /tmp/ch15-cold.log 2>&1
grep '^smoke' /tmp/ch15-cold.log
echo "cold CACHED lines: $(grep -c 'CACHED' /tmp/ch15-cold.log || true)"

# 删掉 builder（本地构建缓存一起没了），再用 registry 里的 cache 重建。
docker buildx rm -f ch15ci >/dev/null
bash ci demo-1 > /tmp/ch15-ci.log 2>&1
grep '^smoke' /tmp/ch15-ci.log
grep 'CACHED' /tmp/ch15-ci.log | head -n 8

docker buildx imagetools inspect localhost:5000/shorty:ci-demo-1 \
  --format '{{.Manifest.Digest}}'
