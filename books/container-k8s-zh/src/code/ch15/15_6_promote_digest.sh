# 先在 dev 通道构建一次（缓存已热）。
bash ci promote-1 > /tmp/ch15-promote.log 2>&1
grep '^smoke' /tmp/ch15-promote.log
tail -n 2 /tmp/ch15-promote.log

# 取 dev tag 的 digest，用 imagetools create 给同一个 manifest 打 prod tag。
dev=$(docker buildx imagetools inspect localhost:5000/shorty:ci-promote-1 \
  --format '{{.Manifest.Digest}}')
docker buildx imagetools create -t localhost:5000/shorty:prod \
  "localhost:5000/shorty:ci-promote-1@$dev"
prod=$(docker buildx imagetools inspect localhost:5000/shorty:prod \
  --format '{{.Manifest.Digest}}')
printf 'dev  %s\n' "$dev"
printf 'prod %s\n' "$prod"
[ "$dev" = "$prod" ] && echo 'same digest'
