# 11.7：最终镜像带上 OCI 标签，版本号从构建参数进入二进制
#> tail: 25
docker buildx build --load --sbom=true \
    --build-arg VERSION=0.1 \
    --build-arg REVISION=9f2c1ab \
    --build-arg CREATED=2026-10-27T00:00:00Z \
    --build-arg SOURCE=https://git.example/shorty \
    -f shorty.Dockerfile -t shorty:0.1 ../app/api

# LABEL 进入镜像配置，Config.Labels 能直接读出来
docker image inspect --format '{{range $k, $v := .Config.Labels}}{{$k}}={{$v}}{{"\n"}}{{end}}' shorty:0.1

# 同一个 VERSION 通过 -ldflags 注入 main.version
docker run --rm shorty:0.1 version
