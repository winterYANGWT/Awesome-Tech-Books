# 10.2：同一份 Dockerfile，先只构建到 dev，再构建默认的 runtime
#> tail: 24
#> timeout: 360
docker buildx build --provenance=false --target dev -f shorty.Dockerfile -t shorty:dev-ch10 --load ../app/api

#> tail: 20
docker buildx build --provenance=false -f shorty.Dockerfile -t shorty:0.1 --load ../app/api

# 两个镜像来自同一个 Dockerfile，但最后一个阶段不同
docker image inspect --format '{{.RepoTags}} size={{.Size}} layers={{len .RootFS.Layers}}' shorty:dev-ch10 shorty:0.1

# dev 阶段有 shell、Go 工具链和源码
docker run --rm shorty:dev-ch10 sh -c 'go version; ls /src'

# runtime 阶段只有二进制
docker run --rm shorty:0.1 version

#> fails
#> expect: exec: "sh": executable file not found
docker run --rm --entrypoint sh shorty:0.1 -c 'echo hello'
