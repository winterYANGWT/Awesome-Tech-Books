# 先取当前 amd64 manifest 的真实 digest。
docker buildx imagetools inspect --raw public.ecr.aws/docker/library/alpine:3.22 \
  | jq -r '.manifests[] | select(.platform.os == "linux" and .platform.architecture == "amd64") | .digest' \
  | tee /tmp/ch02-good-digest

# 把最后一位换成一个不同的十六进制字符，得到不存在的 digest。
GOOD=$(cat /tmp/ch02-good-digest); LAST=${GOOD: -1}; if [ "$LAST" = 0 ]; then NEW=1; else NEW=0; fi; BAD="${GOOD%?}${NEW}"
echo "bad digest: $BAD"

# 拉取这个 digest：registry 不同，错误文本不同；本 lab 的 ECR 端点返回 401。
#> fails
#> expect: (401 Unauthorized|manifest unknown|digest mismatch)
docker pull "public.ecr.aws/docker/library/alpine@${BAD}"

# 用 manifest inspect 定位：这个 digest 根本不存在。
#> fails
#> expect: no such manifest
docker manifest inspect "public.ecr.aws/docker/library/alpine@${BAD}"

# BuildKit 不创建中间镜像：history 有指令，但 ID 是 <missing>，image ls -a 也只有最终镜像。
#> expect: <missing>
docker history --no-trunc ch02-commit-base:1
docker image ls -a ch02-commit-base
