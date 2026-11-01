# 10.5：本地 registry + 两个平台的 manifest list
docker run -d --name registry -p 5000:5000 registry:3.0.0 >/dev/null
#> hide
curl -fsS --retry 30 --retry-all-errors http://localhost:5000/v2/ >/dev/null

# 一次构建两个平台，直接推到 registry
#> tail: 30
#> expect: linux/arm64
#> expect: linux/amd64
#> timeout: 360
docker buildx build --provenance=false --progress=plain \
    --platform linux/amd64,linux/arm64 \
    -f shorty.Dockerfile -t localhost:5000/shorty:0.1 --push ../app/api

# Docker 29 的 containerd 镜像存储能把 manifest list 装进本地镜像库
#> tail: 8
docker buildx build --provenance=false --progress=plain \
    --platform linux/amd64,linux/arm64 \
    -f shorty.Dockerfile -t shorty:multi --load ../app/api
docker image ls shorty

# push 之后本地镜像库没有这个镜像
docker image ls localhost:5000/shorty

docker buildx imagetools inspect localhost:5000/shorty:0.1

# 本机是 amd64，跑 amd64 那个 manifest 正常
docker run --rm --platform linux/amd64 localhost:5000/shorty:0.1 version
