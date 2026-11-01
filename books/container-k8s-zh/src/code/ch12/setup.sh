#!/usr/bin/env bash
# 把 lab 带到第 12 章的起点：
#   * shorty:0.1、shorty-worker:0.1 已构建；
#   * localhost:5000 上有一个不带认证的临时 registry（12.1、12.2 用它）；
#   * 一个单节点 kind 集群（12.5、12.7 用它演示 imagePullSecrets 和 ImagePullBackOff）。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

app_images
pull_images registry:3.0.0 httpd:2.4-alpine

# 12.1、12.2 先用一个最简单的 registry：不挂卷、不配认证。
# 12.3 会把它换成带数据卷的固定实例。
docker rm -f shorty-registry >/dev/null 2>&1 || true
docker run -d --name shorty-registry -p 5000:5000 registry:3.0.0 >/dev/null
for _ in $(seq 1 30); do
    curl -fsS localhost:5000/v2/ >/dev/null 2>&1 && break
    sleep 0.5
done
curl -fsS localhost:5000/v2/ >/dev/null

# 单节点 kind 集群。kind 节点本身是个容器，节点里的 localhost 不是 lab 主机；
# 所以把 localhost:5000 这个 registry 名字映射到 lab 的 docker0 地址，并允许走 HTTP。
cluster_up kind-1n.yaml

REG_IP=$(ip -4 -o addr show docker0 | awk '{print $4}' | cut -d/ -f1)
if [ -z "$REG_IP" ]; then
    printf '找不到 lab 的 docker0 地址，无法让 kind 节点访问 registry\n' >&2
    exit 1
fi
NODE=shorty-control-plane
docker exec "$NODE" mkdir -p /etc/containerd/certs.d/localhost:5000
cat <<EOF | docker exec -i "$NODE" tee /etc/containerd/certs.d/localhost:5000/hosts.toml >/dev/null
server = "http://$REG_IP:5000"

[host."http://$REG_IP:5000"]
  capabilities = ["pull", "resolve"]
  skip_verify = true
EOF

# kind 的 containerd 配置默认不读 certs.d，补上 config_path 再重启。
if ! docker exec "$NODE" grep -qF 'config_path = "/etc/containerd/certs.d"' /etc/containerd/config.toml; then
    docker exec "$NODE" sh -c 'printf "\n[plugins.\"io.containerd.grpc.v1.cri\".registry]\n  config_path = \"/etc/containerd/certs.d\"\n" >> /etc/containerd/config.toml'
fi
docker exec "$NODE" systemctl restart containerd
kubectl wait --for=condition=Ready node --all --timeout=180s >/dev/null

kubectl create namespace shorty --dry-run=client -o yaml | kubectl apply -f - >/dev/null
for _ in $(seq 1 30); do
    kubectl -n shorty get serviceaccount default >/dev/null 2>&1 && break
    sleep 1
done
kubectl -n shorty get serviceaccount default >/dev/null
