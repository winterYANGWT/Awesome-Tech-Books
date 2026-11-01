# index（manifest list）按 platform 列出每个 manifest 的 digest。
#> expect: linux/arm64
docker buildx imagetools inspect --raw public.ecr.aws/docker/library/alpine:3.22 \
  | jq -r '.manifests[]
      | select(.platform.os == "linux")
      | select(.platform.architecture == "amd64" or .platform.architecture == "arm64")
      | "\(.platform.os)/\(.platform.architecture)/\(.platform.variant // "-")  \(.digest)"'

# 本地镜像记录里也能看到两个平台：一个已有层，另一个只是 index 里的条目。
docker image ls --tree alpine:3.22

# 选 arm64 的 manifest 并运行：这台 amd64 机器没有注册 qemu，内核拒绝执行。
#> fails
#> expect: exec format error
docker run --rm --platform linux/arm64 alpine:3.22 uname -m

# 选回 amd64，正常输出。
#> expect: x86_64
docker run --rm --platform linux/amd64 alpine:3.22 uname -m
