#!/usr/bin/env bash
# 第 23 章的起点：3 节点集群 + s3-stateful。
# api 2 副本、postgres StatefulSet + headless Service、redis、worker 都在 shorty 命名空间里。
# 另外准备 busybox 客户端镜像，以及 cloud-provider-kind（演示 type: LoadBalancer）。
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

# 清掉上一次运行可能留下的 LoadBalancer 控制器，避免它继续观察新集群。
pkill -f /usr/local/bin/cloud-provider-kind 2>/dev/null || true

stage_up s3-stateful kind-3n.yaml
pull_images busybox:1.37

# cloud-provider-kind 不放进 lab 镜像，第一次运行时下载，之后从 build/scratch/ch23 复用。
# 只写 build/scratch 下的临时文件，不修改 lab 或镜像。
cpk_version=v0.12.0
cpk_dir=$(cd ../../.. && pwd)/build/scratch/ch23
if [ ! -x "$cpk_dir/cloud-provider-kind" ]; then
    mkdir -p "$cpk_dir"
    curl -fsSLo "$cpk_dir/cloud-provider-kind.tgz" \
        "https://github.com/kubernetes-sigs/cloud-provider-kind/releases/download/${cpk_version}/cloud-provider-kind_${cpk_version#v}_linux_amd64.tar.gz"
    tar xzf "$cpk_dir/cloud-provider-kind.tgz" -C "$cpk_dir" cloud-provider-kind
fi
install -m 0755 "$cpk_dir/cloud-provider-kind" /usr/local/bin/cloud-provider-kind

# 让 LoadBalancer 控制器在后台跑。它用当前 kubeconfig 找到 kind 集群，
# 再通过 lab 里的 Docker 创建 Envoy 容器做数据面。
nohup cloud-provider-kind >/tmp/cloud-provider-kind.log 2>&1 &
