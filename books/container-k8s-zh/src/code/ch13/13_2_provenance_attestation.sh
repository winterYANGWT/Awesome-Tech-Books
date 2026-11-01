# 13.2：用 mode=max 记录构建参数和材料，并显式设置 builder-id
REG=$(cat /tmp/ch13-registry)
cd ../app/api
#> tail: 14
#> expect: exporting attestation manifest
docker buildx build --sbom=true \
  --provenance=mode=max,version=v0.2,builder-id=https://build.shorty.example/ci/run-42 \
  --push -t "$REG/shorty:0.1" .

DIGEST=$(docker buildx imagetools inspect "$REG/shorty:0.1" --format '{{.Manifest.Digest}}')
printf '%s\n' "$DIGEST" > /tmp/ch13-digest

#> expect: unknown/unknown
docker buildx imagetools inspect "$REG/shorty:0.1"

docker buildx imagetools inspect "$REG/shorty:0.1" \
  --format '{{json .Provenance.SLSA}}' > /tmp/ch13-slsa.json

#> expect: https://build.shorty.example/ci/run-42
jq -r '"buildType: \(.buildType)", "builder: \(.builder.id)",
       (.materials[] | "material: \(.uri)"),
       (.invocation.parameters.args | to_entries[] | "arg: \(.key)=\(.value)"),
       "vcs: \(.metadata["https://mobyproject.org/buildkit@v1#metadata"].vcs // "无")"' \
  /tmp/ch13-slsa.json
