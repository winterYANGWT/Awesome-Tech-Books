#!/usr/bin/env bash
# 第 34 章起点：s4-prod 集群、本地 registry（OCI chart 与 digest 用）。
# Argo CD 和 Gitea 在 34.3 的示例里安装，这样读者能看到安装过程。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

ROOT=$(cd ../../.. && pwd)
rm -rf "$ROOT/build/scratch/ch34"
mkdir -p "$ROOT/build/scratch/ch34"

stage_up s4-prod

# HPA 也管 api 的 replicas；本章要看 GitOps 对账，先把它移出实验。
"${KUBECTL[@]}" -n shorty delete hpa api --ignore-not-found >/dev/null

# 34.3 的 Argo CD 和 Gitea 镜像先放进节点，示例输出里就不会混进下载进度。
pull_images quay.io/argoproj/argocd:v3.5.6 gitea/gitea:28.2.0 ecr-public.aws.com/docker/library/redis:8.6.4-alpine

# 起一个不带认证的本地 registry。34.2 推 OCI chart，34.6 用它的 digest 做提升。
docker rm -f shorty-registry >/dev/null 2>&1 || true
docker run -d --name shorty-registry -p 5000:5000 registry:3.0.0 >/dev/null
for _ in $(seq 1 60); do
    curl -fsS localhost:5000/v2/ >/dev/null 2>&1 && break
    sleep 0.5
done
curl -fsS localhost:5000/v2/ >/dev/null

# kind 节点是容器，节点里的 localhost 不是 lab 主机。把 localhost:5000 指到 docker0，
# 并允许 plain HTTP；后面的 Pod 才能从 registry 拉 shorty。
NODE=$("${KUBECTL[@]}" get nodes -o jsonpath='{.items[0].metadata.name}')
REG_IP=$(ip -4 -o addr show docker0 | awk '{print $4}' | cut -d/ -f1)
if [ -z "$NODE" ] || [ -z "$REG_IP" ]; then
    printf '找不到 kind 节点或 docker0 地址\n' >&2
    exit 1
fi
docker exec "$NODE" mkdir -p /etc/containerd/certs.d/localhost:5000
cat <<EOF | docker exec -i "$NODE" tee /etc/containerd/certs.d/localhost:5000/hosts.toml >/dev/null
server = "http://$REG_IP:5000"

[host."http://$REG_IP:5000"]
  capabilities = ["pull", "resolve"]
  skip_verify = true
EOF
if ! docker exec "$NODE" grep -qF 'config_path = "/etc/containerd/certs.d"' /etc/containerd/config.toml; then
    docker exec "$NODE" sh -c 'printf "\n[plugins.\"io.containerd.grpc.v1.cri\".registry]\n  config_path = \"/etc/containerd/certs.d\"\n" >> /etc/containerd/config.toml'
fi
docker exec "$NODE" systemctl restart containerd
"${KUBECTL[@]}" wait --for=condition=Ready node --all --timeout=180s >/dev/null
"${KUBECTL[@]}" wait --for=condition=Available deployment --all -n shorty --timeout=180s >/dev/null

# 给本地镜像一个 registry 里的名字并推上去，34.2/34.3/34.6 用这个仓库。
docker tag shorty:0.1 localhost:5000/shorty:0.1
docker push -q localhost:5000/shorty:0.1
docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' localhost:5000/shorty:0.1
