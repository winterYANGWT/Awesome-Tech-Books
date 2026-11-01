# 同一个应用：单阶段镜像和 scratch 多阶段镜像
#> tail: 24
docker build --progress=plain --provenance=false --no-cache -f multi.Dockerfile -t shorty:scratch ../app/api

# DISK USAGE 是解压后占的磁盘，CONTENT SIZE 是压缩后的内容大小
docker image ls shorty:dev
docker image ls shorty:scratch

# scratch 里没有 shell，只有静态二进制和 CA 证书
docker run --rm shorty:scratch version

#> fails
#> expect: exec: "sh": executable file not found
docker run --rm --entrypoint sh shorty:scratch -c 'echo hello'
