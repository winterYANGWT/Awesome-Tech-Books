# 13.1：带 SBOM attestation 构建并推送；先关掉 provenance，避免两种 attestation 混在一起
REG=$(cat /tmp/ch13-registry)
cd ../app/api
#> tail: 12
#> expect: exporting attestation manifest
docker buildx build --sbom=true --provenance=false --push -t "$REG/shorty:0.1" .

DIGEST=$(docker buildx imagetools inspect "$REG/shorty:0.1" --format '{{.Manifest.Digest}}')
printf '%s\n' "$DIGEST" > /tmp/ch13-digest

#> expect: unknown/unknown
#> expect: attestation-manifest
docker buildx imagetools inspect "$REG/shorty:0.1"

#> expect: SPDX-2
docker buildx imagetools inspect "$REG/shorty:0.1" --format '{{json .SBOM.SPDX}}' \
  | jq -r '.spdxVersion, "packages: \(.packages | length)", (.packages[] | "\(.name)@\(.versionInfo)")' \
  | head -12
