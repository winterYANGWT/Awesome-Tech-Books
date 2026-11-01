#!/usr/bin/env bash
# 第 13 章的起点：本地 TLS registry 里有未签名的 shorty:0.1，cosign、Trivy 和策略控制器可用。
set -euo pipefail

source ../lib/common.sh

REG_IP=$(ip -4 -o addr show docker0 | awk '{print $4}' | cut -d/ -f1)
if [ -z "$REG_IP" ]; then
  printf '找不到 lab 内层 docker0 的地址，无法让 kind 节点访问 registry\n' >&2
  exit 1
fi
REG="$REG_IP:5000"
ROOT=$(cd ../../.. && pwd)
TLS="$ROOT/build/scratch/ch13/tls"
mkdir -p "$TLS" /tmp/ch13-keys
printf '%s\n' "$REG" > /tmp/ch13-registry

# cosign 2.x：本章用 --tlog-upload=false 和旧式 .sig 标签，policy-controller 也能直接读。
if ! command -v cosign >/dev/null 2>&1 || ! cosign version 2>/dev/null | grep -q 'GitVersion:.*v2'; then
  _say "安装 cosign v2.4.1"
  curl -fsSLo /tmp/cosign-linux-amd64 \
    https://github.com/sigstore/cosign/releases/download/v2.4.1/cosign-linux-amd64
  curl -fsSLo /tmp/cosign_checksums.txt \
    https://github.com/sigstore/cosign/releases/download/v2.4.1/cosign_checksums.txt
  (cd /tmp && grep ' cosign-linux-amd64$' cosign_checksums.txt | sha256sum -c - >/dev/null)
  install -m 0755 /tmp/cosign-linux-amd64 /usr/local/bin/cosign
fi

apk add --no-cache openssl ca-certificates >/dev/null 2>&1

# registry 用自签证书。kind 节点和 policy-controller 都通过 172.x 的 docker0 网关访问它，
# 所以证书 SAN 写 IP，而不是 localhost。
_say "生成本地 registry 的自签证书"
rm -rf "$TLS"
mkdir -p "$TLS"
openssl req -x509 -newkey rsa:2048 -nodes -days 2 \
  -keyout "$TLS/ca.key" -out "$TLS/ca.crt" -subj "/CN=shorty-lab-ca" >/dev/null 2>&1
openssl req -newkey rsa:2048 -nodes \
  -keyout "$TLS/server.key" -out "$TLS/server.csr" -subj "/CN=$REG_IP" >/dev/null 2>&1
printf 'subjectAltName=IP:%s\n' "$REG_IP" > "$TLS/ext.cnf"
openssl x509 -req -in "$TLS/server.csr" -CA "$TLS/ca.crt" -CAkey "$TLS/ca.key" \
  -CAcreateserial -days 2 -extfile "$TLS/ext.cnf" -out "$TLS/server.crt" >/dev/null 2>&1

# Docker CLI/daemon 和 cosign 都从系统信任库和 certs.d 读 CA。
mkdir -p "/etc/docker/certs.d/$REG"
cp "$TLS/ca.crt" "/etc/docker/certs.d/$REG/ca.crt"
mkdir -p /usr/local/share/ca-certificates
cp "$TLS/ca.crt" /usr/local/share/ca-certificates/shorty-lab.crt
update-ca-certificates >/dev/null 2>&1

_say "启动本地 registry（HTTPS，$REG）"
docker rm -f registry >/dev/null 2>&1 || true
docker run -d --name registry -p 5000:5000 \
  -e REGISTRY_HTTP_TLS_CERTIFICATE=/certs/server.crt \
  -e REGISTRY_HTTP_TLS_KEY=/certs/server.key \
  -v "$TLS:/certs:ro" registry:3.0.0 >/dev/null
for _ in $(seq 1 30); do
  curl -fsS --cacert "$TLS/ca.crt" "https://$REG/v2/" >/dev/null 2>&1 && break
  sleep 1
done
curl -fsS --cacert "$TLS/ca.crt" "https://$REG/v2/" >/dev/null

_say "预拉本章要用的镜像"
pull_images alpine:3.10 alpine:3.22 registry:3.0.0 \
  docker/buildkit-syft-scanner:stable-1 \
  aquasec/trivy:0.76.1 \
  ghcr.io/sigstore/policy-controller/policy-controller@sha256:0bcd60beb93f4427c29cf3a669743caf58490e98ded4380c33c09f092734a6ab

# 起点：本地构建的 shorty:0.1 推到 registry；unsigned 这个 tag 留给后面的失败示例。
_say "构建并推送未签名的 shorty:0.1"
app_images
docker tag shorty:0.1 "$REG/shorty:0.1"
docker tag shorty:0.1 "$REG/shorty:unsigned"
docker push -q "$REG/shorty:0.1" >/dev/null
docker push -q "$REG/shorty:unsigned" >/dev/null

# 扫描数据库约 130 MB，缓存到 build/scratch/ch13，重跑本章时不再下载。
_say "预热 Trivy 漏洞库"
docker run --rm -v "$ROOT/build/scratch/ch13/trivy-cache:/root/.cache/trivy" \
  aquasec/trivy:0.76.1 image --download-db-only --quiet >/dev/null
