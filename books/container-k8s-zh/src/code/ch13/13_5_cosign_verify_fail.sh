# 13.5：把 tag 指到未签名镜像，tag 验签失败；digest 的签名还在
REG=$(cat /tmp/ch13-registry)
SIGNED=$(cat /tmp/ch13-digest)
UNSIGNED=$(docker buildx imagetools inspect "$REG/shorty:unsigned" --format '{{.Manifest.Digest}}')
cd /tmp/ch13-keys
printf 'signed:   %s\nunsigned: %s\n' "$SIGNED" "$UNSIGNED"

docker buildx imagetools create -t "$REG/shorty:0.1" "$REG/shorty@$UNSIGNED"

#> fails
#> expect: no signatures found
cosign verify --key cosign.pub --insecure-ignore-tlog "$REG/shorty:0.1"

docker buildx imagetools create -t "$REG/shorty:0.1" "$REG/shorty@$SIGNED"
cosign verify --key cosign.pub --insecure-ignore-tlog "$REG/shorty@$SIGNED" >/dev/null 2>&1 \
  && echo "digest 仍然验签通过"
