# 13.9：盘点镜像的签名、SBOM 和 provenance，每一项都对着 digest
REG=$(cat /tmp/ch13-registry)
DIGEST=$(cat /tmp/ch13-digest)
cd /tmp/ch13-keys

#> expect: .sig
#> expect: .att
cosign tree "$REG/shorty@$DIGEST"

#> expect: sha256:[0-9a-f]{64}
cosign verify --key cosign.pub --insecure-ignore-tlog "$REG/shorty@$DIGEST" \
  | jq -r 'if type=="array" then .[0] else . end | .critical.image["docker-manifest-digest"]'

#> expect: https://spdx.dev/Document
cosign verify-attestation --key attest.pub --insecure-ignore-tlog \
  --type spdxjson "$REG/shorty@$DIGEST" \
  |  jq -r 'if type=="array" then .[0].payload else .payload end' | base64 -d | jq -r '.predicateType'

#> expect: https://slsa.dev/provenance/v0.2
cosign verify-attestation --key attest.pub --insecure-ignore-tlog \
  --type slsaprovenance "$REG/shorty@$DIGEST" \
  |  jq -r 'if type=="array" then .[0].payload else .payload end' | base64 -d | jq -r '.predicateType, .predicate.builder.id'

curl -fsS --cacert /book/build/scratch/ch13/tls/ca.crt "https://$REG/v2/shorty/tags/list" \
  | jq -r '.tags[]'
