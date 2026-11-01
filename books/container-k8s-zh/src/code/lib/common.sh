#!/usr/bin/env bash
# 各章 setup.sh 共用的函数。用法：
#   source ../lib/common.sh
#   cluster_up                 # 单节点集群
#   app_images                 # 构建 shorty 镜像并加载进集群
#   pull_images postgres:18.6-alpine
#   stage_up s2-data           # 建集群 + 装前置组件 + 加载镜像 + apply + 等就绪
#
# 函数默认只打印一两行进度；失败时返回非 0，由 stage_up 打印 Event 和 Pod 状态。
# 需要调用方自己开启 set -euo pipefail（章节目录里的 setup.sh 都这么做）。

# shellcheck shell=bash

SHORTY_CODE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
SHORTY_APP_DIR="$SHORTY_CODE_DIR/app"
SHORTY_CLUSTER_DIR="$SHORTY_CODE_DIR/cluster"
SHORTY_STAGES_DIR="$SHORTY_CODE_DIR/stages"

KIND_CLUSTER=${KIND_CLUSTER:-shorty}
KUBECTL=(kubectl --context "kind-$KIND_CLUSTER")

_say() { printf '==> %s\n' "$*"; }

_cluster_exists() {
  kind get clusters 2>/dev/null | grep -Fxq "$KIND_CLUSTER"
}

_resolve_config() {
  local name=$1 candidate
  for candidate in "$name" "$name.yaml" "$SHORTY_CLUSTER_DIR/$name" "$SHORTY_CLUSTER_DIR/$name.yaml"; do
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  printf '找不到 kind 配置：%s\n' "$name" >&2
  return 1
}

_wait_apiserver() {
  local i
  for i in $(seq 1 90); do
    if "${KUBECTL[@]}" get --raw=/readyz >/dev/null 2>&1; then
      return 0
    fi
    sleep 2
  done
  printf '等待 apiserver 就绪超时\n' >&2
  return 1
}

_wait_nodes_ready() {
  local i total ready
  for i in $(seq 1 150); do
    total=$("${KUBECTL[@]}" get nodes --no-headers 2>/dev/null | wc -l)
    ready=$("${KUBECTL[@]}" get nodes \
      -o jsonpath='{range .items[*]}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}' \
      2>/dev/null | grep -c '^True$' || true)
    if [ "$total" -gt 0 ] && [ "$ready" -eq "$total" ]; then
      return 0
    fi
    sleep 2
  done
  printf '等待节点 Ready 超时（%s/%s）\n' "${ready:-0}" "${total:-0}" >&2
  return 1
}

# cluster_up [配置文件名，默认 kind-1n.yaml]
# 删掉同名旧集群再建，保证节点数和 CNI 配置与参数一致。
cluster_up() {
  local config=${1:-kind-1n.yaml} config_path out
  if [ -z "${KIND_NODE_IMAGE:-}" ]; then
    printf 'KIND_NODE_IMAGE 未设置；lab 镜像里应已提供 kindest/node 的固定 digest\n' >&2
    return 1
  fi
  config_path=$(_resolve_config "$config") || return 1

  if _cluster_exists; then
    _say "删除已有集群 $KIND_CLUSTER"
    if ! out=$(kind delete cluster --name "$KIND_CLUSTER" 2>&1); then
      printf '%s\n' "$out" >&2
      return 1
    fi
  fi

  _say "创建集群 $KIND_CLUSTER（$(basename "$config_path")）"
  if ! out=$(kind create cluster --name "$KIND_CLUSTER" \
      --image "$KIND_NODE_IMAGE" --config "$config_path" 2>&1); then
    printf '%s\n' "$out" >&2
    return 1
  fi
  kind export kubeconfig --name "$KIND_CLUSTER" >/dev/null 2>&1 || {
    printf '导出 kubeconfig 失败\n' >&2
    return 1
  }

  if grep -q 'disableDefaultCNI:[[:space:]]*true' "$config_path"; then
    _say "配置禁用了默认 CNI，跳过节点 Ready 等待"
    _wait_apiserver || return 1
  else
    _wait_nodes_ready || return 1
  fi
}

# app_images
# 构建 api 与 worker 镜像；集群已存在时用 kind load 放进节点。
app_images() {
  _say "构建 shorty:0.1 与 shorty-worker:0.1"
  if ! docker build -q -t shorty:0.1 "$SHORTY_APP_DIR/api" >/dev/null; then
    printf '构建 shorty:0.1 失败\n' >&2
    return 1
  fi
  if ! docker build -q -t shorty-worker:0.1 "$SHORTY_APP_DIR/worker" >/dev/null; then
    printf '构建 shorty-worker:0.1 失败\n' >&2
    return 1
  fi
  if _cluster_exists; then
    _say "把 shorty 镜像加载进集群"
    if ! kind load docker-image --name "$KIND_CLUSTER" \
        shorty:0.1 shorty-worker:0.1 >/dev/null; then
      printf 'kind load docker-image 失败\n' >&2
      return 1
    fi
  fi
}

# Docker 29 的 containerd 镜像存储里，postgres/redis 这类多平台镜像
# 直接 kind load docker-image 会因为缺少其他平台的 blob 而失败。
# 用 docker save --platform 只导出本机平台，再走 image-archive。
_kind_load_image() {
  local image=$1 platform tar rc=0
  if kind load docker-image --name "$KIND_CLUSTER" "$image" >/dev/null 2>&1; then
    return 0
  fi
  platform=$(docker version --format '{{.Server.Os}}/{{.Server.Arch}}' 2>/dev/null || true)
  [ -n "$platform" ] || platform=linux/amd64
  tar=$(mktemp "${TMPDIR:-/tmp}/cbook-image-XXXXXX.tar")
  if docker save --platform "$platform" -o "$tar" "$image" >/dev/null 2>&1 \
      && kind load image-archive --name "$KIND_CLUSTER" "$tar" >/dev/null 2>&1; then
    rc=0
  else
    rc=1
  fi
  rm -f "$tar"
  if [ "$rc" -ne 0 ]; then
    printf '加载 %s 到集群失败\n' "$image" >&2
  fi
  return "$rc"
}

# pull_images 镜像…
# 静默预拉；集群已存在时也加载进去，让后面的示例输出里没有下载进度。
pull_images() {
  local image
  if [ "$#" -eq 0 ]; then
    return 0
  fi
  for image in "$@"; do
    _say "预拉 $image"
    if ! docker pull -q "$image" >/dev/null; then
      printf '拉取 %s 失败\n' "$image" >&2
      return 1
    fi
  done
  if _cluster_exists; then
    _say "把预拉镜像加载进集群"
    for image in "$@"; do
      _kind_load_image "$image" || return 1
    done
  fi
}

_install_metrics_server() {
  local version=v0.9.0
  _say "安装 metrics-server $version"
  if ! "${KUBECTL[@]}" apply -f \
      "https://github.com/kubernetes-sigs/metrics-server/releases/download/$version/components.yaml" \
      >/dev/null; then
    printf '安装 metrics-server 失败\n' >&2
    return 1
  fi
  # kind 的 kubelet 用自签证书，metrics-server 默认不信任。
  if ! "${KUBECTL[@]}" -n kube-system get deployment metrics-server \
      -o jsonpath='{.spec.template.spec.containers[0].args}' \
      2>/dev/null | grep -q -- '--kubelet-insecure-tls'; then
    "${KUBECTL[@]}" -n kube-system patch deployment metrics-server --type=json \
      -p='[{"op":"add","path":"/spec/template/spec/containers/0/args/-","value":"--kubelet-insecure-tls"}]' \
      >/dev/null
  fi
  if ! "${KUBECTL[@]}" -n kube-system rollout status deployment/metrics-server \
      --timeout=180s >/dev/null; then
    printf 'metrics-server 未就绪\n' >&2
    return 1
  fi
}

_install_gateway() {
  local api_version=v1.6.3 ngf_version=2.8.0
  _say "安装 Gateway API $api_version CRD"
  if ! "${KUBECTL[@]}" apply -f \
      "https://github.com/kubernetes-sigs/gateway-api/releases/download/$api_version/standard-install.yaml" \
      >/dev/null; then
    printf '安装 Gateway API CRD 失败\n' >&2
    return 1
  fi
  _say "安装 NGINX Gateway Fabric $ngf_version"
  if ! helm upgrade --install ngf \
      oci://ghcr.io/nginx/charts/nginx-gateway-fabric \
      --version "$ngf_version" \
      --namespace nginx-gateway --create-namespace \
      --wait --timeout 300s >/dev/null; then
    printf '安装 NGINX Gateway Fabric 失败\n' >&2
    return 1
  fi
}

_wait_workloads() {
  local timeout=${STAGE_WAIT_TIMEOUT:-300} deadline name desired ready job failed
  deadline=$((SECONDS + timeout))
  while :; do
    local ok=1
    while read -r name desired ready; do
      [ -n "$name" ] || continue
      [ "${ready:-0}" -ge "${desired:-1}" ] || ok=0
    done < <("${KUBECTL[@]}" -n shorty get deployment -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.spec.replicas}{" "}{.status.availableReplicas}{"\n"}{end}' 2>/dev/null)

    while read -r name desired ready; do
      [ -n "$name" ] || continue
      [ "${ready:-0}" -ge "${desired:-1}" ] || ok=0
    done < <("${KUBECTL[@]}" -n shorty get statefulset -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.spec.replicas}{" "}{.status.readyReplicas}{"\n"}{end}' 2>/dev/null)

    for job in $("${KUBECTL[@]}" -n shorty get job -o name 2>/dev/null); do
      failed=$("${KUBECTL[@]}" -n shorty get "$job" -o jsonpath='{.status.conditions[?(@.type=="Failed")].status}' 2>/dev/null)
      if [ "$failed" = True ]; then
        printf '%s 失败\n' "$job" >&2
        return 1
      fi
      ready=$("${KUBECTL[@]}" -n shorty get "$job" -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}' 2>/dev/null)
      [ "$ready" = True ] || ok=0
    done

    [ "$ok" -eq 1 ] && return 0
    if [ "$SECONDS" -ge "$deadline" ]; then
      printf '等待 Deployment/StatefulSet/Job 就绪超时（%ss）\n' "$timeout" >&2
      return 1
    fi
    sleep 2
  done
}

_wait_gateway() {
  local timeout=${STAGE_WAIT_TIMEOUT:-300} deadline
  deadline=$((SECONDS + timeout))
  # Gateway 数据面的 Deployment 是 NGF 控制器异步创建的，先等它出现。
  while [ -z "$("${KUBECTL[@]}" -n shorty get deployment \
      -l gateway.networking.k8s.io/gateway-name=shorty-gw -o name 2>/dev/null)" ]; do
    if [ "$SECONDS" -ge "$deadline" ]; then
      printf '等待 Gateway 数据面 Deployment 超时（%ss）\n' "$timeout" >&2
      return 1
    fi
    sleep 2
  done
  "${KUBECTL[@]}" -n shorty wait --for=condition=Available \
    -l gateway.networking.k8s.io/gateway-name=shorty-gw \
    --timeout="${timeout}s" deployment >/dev/null
  "${KUBECTL[@]}" -n shorty wait --for=condition=Programmed \
    gateway/shorty-gw --timeout="${timeout}s" >/dev/null
}

_stage_diagnostics() {
  _say "stage 启动失败，最近的事件："
  "${KUBECTL[@]}" get events -A --sort-by=.lastTimestamp 2>&1 | tail -n 40 || true
  _say "Pod 状态："
  "${KUBECTL[@]}" get pods -A -o wide 2>&1 | tail -n 40 || true
}

_stage_up_impl() {
  local stage=$1 config=$2
  cluster_up "$config" || return 1
  case "$stage" in
    s4-prod)
      _install_metrics_server || return 1
      _install_gateway || return 1
      ;;
  esac
  app_images || return 1
  case "$stage" in
    s2-data | s3-stateful | s4-prod)
      # 先把数据库和队列镜像放进节点，apply 之后 Pod 不必再等 registry。
      pull_images postgres:18.6-alpine redis:8.8.4-alpine || return 1
      ;;
  esac
  _say "应用 stages/$stage 清单"
  "${KUBECTL[@]}" apply -f "$SHORTY_STAGES_DIR/$stage" >/dev/null || return 1
  _wait_workloads || return 1
  if [ "$stage" = s4-prod ]; then
    _wait_gateway || return 1
  fi
}

# stage_up <stage> [配置文件名，默认 kind-1n.yaml]
# 从空 lab 建集群、装该 stage 的前置组件、构建并加载镜像、apply 清单、等就绪。
stage_up() {
  local stage=${1:-} config=${2:-${KIND_CONFIG:-kind-1n.yaml}}
  if [ -z "$stage" ]; then
    printf '用法: stage_up <stage> [kind 配置]\n' >&2
    return 2
  fi
  if [ ! -d "$SHORTY_STAGES_DIR/$stage" ]; then
    printf '未知 stage：%s\n' "$stage" >&2
    return 2
  fi
  if ! ( set -uo pipefail; _stage_up_impl "$stage" "$config" ); then
    _stage_diagnostics
    return 1
  fi
}
