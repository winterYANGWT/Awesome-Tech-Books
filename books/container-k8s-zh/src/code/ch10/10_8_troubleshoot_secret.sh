# 10.7：故意把 secret 写成 ARG，token 会留在 history 和层里
#> tail: 8
docker buildx build --provenance=false --progress=plain \
    --build-arg TOKEN="$(cat /tmp/ch10-token)" \
    -f leak.Dockerfile -t shorty:leak --load .

TOKEN=$(cat /tmp/ch10-token)
# 把 token 的值替换掉再显示，证明它确实在 history 里
docker history --no-trunc --format '{{.CreatedBy}}' shorty:leak \
    | grep -F "$TOKEN" | sed "s/$TOKEN/<redacted>/g" | head -3

# 层里的文件也能搜到
docker run --rm --entrypoint sh shorty:leak \
    -c 'test -s /token.txt && echo "token.txt exists: $(wc -c < /token.txt) bytes"'
docker run --rm -e TOKEN="$TOKEN" --entrypoint sh shorty:leak \
    -c 'grep -lF "$TOKEN" /token.txt'

# 改成 secret mount 后，同一个 token 不再出现在 history 和镜像里
#> tail: 10
docker buildx build --provenance=false --progress=plain \
    --secret id=git_token,src=/tmp/ch10-token \
    -f fixed.Dockerfile -t shorty:fixed --load .
docker run --rm shorty:fixed
docker history --no-trunc --format '{{.CreatedBy}}' shorty:fixed | grep -F "$TOKEN" || echo "history: token not found"
docker run --rm -e TOKEN="$TOKEN" --entrypoint sh shorty:fixed \
    -c 'grep -rlF "$TOKEN" /status.txt /etc /root /usr /bin /sbin /tmp 2>/dev/null || echo "image: token not found"'
