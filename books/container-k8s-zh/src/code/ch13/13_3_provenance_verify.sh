# 13.3：BuildKit 的 attestation 只是随镜像推上去的 JSON，要自己用 cosign 签一次才能验签
REG=$(cat /tmp/ch13-registry)
DIGEST=$(cat /tmp/ch13-digest)
cd /tmp/ch13-keys
rm -f attest.key attest.pub
export COSIGN_PASSWORD=ch13-lab-password
cosign generate-key-pair --output-key-prefix attest

docker buildx imagetools inspect "$REG/shorty:0.1" \
  --format '{{json .SBOM.SPDX}}' > /tmp/ch13-sbom.json

cosign attest --key attest.key --yes --tlog-upload=false \
  --type slsaprovenance --predicate /tmp/ch13-slsa.json "$REG/shorty@$DIGEST"
cosign attest --key attest.key --yes --tlog-upload=false \
  --type spdxjson --predicate /tmp/ch13-sbom.json "$REG/shorty@$DIGEST"

#> expect: https://build.shorty.example/ci/run-42
cosign verify-attestation --key attest.pub --insecure-ignore-tlog \
  --type slsaprovenance "$REG/shorty@$DIGEST" \
  |  jq -r 'if type=="array" then .[0].payload else .payload end' | base64 -d | jq -r '.predicate.builder.id'

#> fails
#> expect: no matching attestations
cosign verify-attestation --key attest.pub --insecure-ignore-tlog \
  --type slsaprovenance "$REG/shorty:unsigned"
