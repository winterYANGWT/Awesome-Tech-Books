# 13.4：签名对象是 digest；签名单独存成 sha256-<digest>.sig 标签
REG=$(cat /tmp/ch13-registry)
DIGEST=$(cat /tmp/ch13-digest)
cd /tmp/ch13-keys
rm -f cosign.key cosign.pub
export COSIGN_PASSWORD=ch13-lab-password
cosign generate-key-pair

cosign sign --key cosign.key --yes --tlog-upload=false "$REG/shorty@$DIGEST"

#> expect: The signatures were verified against the specified public key
cosign verify --key cosign.pub --insecure-ignore-tlog "$REG/shorty@$DIGEST" \
  | jq -r 'if type=="array" then .[0] else . end | .critical.image["docker-manifest-digest"], .critical.identity["docker-reference"]'

#> expect: sha256-
cosign tree "$REG/shorty@$DIGEST"
