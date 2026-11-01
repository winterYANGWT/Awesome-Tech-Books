# 10.4：私有依赖放在制品库里，构建时用 secret 取 token
# 上下文 secretapp/ 里是 main.go 和 go.mod；私有模块源码在 private/secretlib/go.mod 和 secretlib.go
# 先用一个错误的 token，制品库返回 401
printf 'wrong-token' > /tmp/ch10-wrong-token

#> fails
#> tail: 22
#> expect: 401
#> timeout: 300
docker buildx build --provenance=false --progress=plain \
    --secret id=git_token,src=/tmp/ch10-wrong-token \
    -f secret.Dockerfile -t shorty:secret --load secretapp

# 换成正确的 token，构建通过
#> tail: 18
#> timeout: 300
docker buildx build --provenance=false --progress=plain \
    --secret id=git_token,src=/tmp/ch10-token \
    -f secret.Dockerfile -t shorty:secret --load secretapp

docker run --rm shorty:secret

# token 不在 docker history 里，也不在镜像的文件系统里
TOKEN=$(cat /tmp/ch10-token)
docker history --no-trunc shorty:secret
docker history --no-trunc --format '{{.CreatedBy}}' shorty:secret | grep -F "$TOKEN" || echo "history: token not found"
docker run --rm -e TOKEN="$TOKEN" --entrypoint sh shorty:secret \
    -c 'grep -rlF "$TOKEN" /secretapp /etc /usr /bin /sbin /root /tmp 2>/dev/null || echo "image: token not found"'
