# 9.5：ENV 把 token 写进镜像配置，ARG 只活到构建结束
#> tail: 6
docker build --progress=plain --provenance=false -f leak.Dockerfile -t shorty:leak ../app/api

# ENV 的值留在镜像的 Config.Env 里
docker image inspect --format '{{json .Config.Env}}' shorty:leak

# docker history 的记录可以追溯到这条 ENV
docker history --no-trunc shorty:leak > /tmp/leak-history.txt
head -6 /tmp/leak-history.txt

# 同样的值改用 ARG 传入：构建期能看见，镜像配置里没有
docker build --progress=plain --provenance=false --build-arg BUILD_TOKEN=shorty-demo-token \
  -f arg.Dockerfile -t shorty:arg ../app/api
docker image inspect --format '{{json .Config.Env}}' shorty:arg

# 安全门禁：镜像里的进程能看到 TOKEN，这个检查应当失败
#> fails
#> expect: TOKEN 仍在镜像里
docker run --rm shorty:leak sh -c 'if [ -n "$TOKEN" ]; then echo "TOKEN 仍在镜像里"; exit 1; fi'
