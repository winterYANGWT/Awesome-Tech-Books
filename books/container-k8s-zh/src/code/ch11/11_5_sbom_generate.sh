# 11.5：SBOM 作为 attestation 跟着镜像走，先推到一个临时 registry 才能看索引
#> hide
{
    docker rm -f ch11-registry >/dev/null 2>&1 || true
    docker run -d --name ch11-registry -p 5000:5000 registry:3.0.0 >/dev/null
    # registry 起来之前 curl 会 reset，--retry 会重试；这段准备不放进正文
    curl -fsS --retry 30 --retry-all-errors --retry-delay 1 http://localhost:5000/v2/ >/dev/null
}

# -q 只留推送后的 digest；--sbom=true 让 BuildKit 在构建时生成 SPDX 文档
docker buildx build -q --sbom=true --push \
    -t localhost:5000/shorty:0.1-sbom -f shorty.Dockerfile ../app/api

# 索引里多出来的 unknown/unknown 就是 attestation manifest
#> expect: attestation-manifest
docker buildx imagetools inspect localhost:5000/shorty:0.1-sbom
