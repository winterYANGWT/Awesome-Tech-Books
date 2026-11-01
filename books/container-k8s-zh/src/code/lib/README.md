# lib：各章 setup.sh 的公共函数

`common.sh` 由章节目录里的 `setup.sh` 引入：

```bash
#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
source ../lib/common.sh

stage_up s2-data
pull_images nginx:1.29-alpine
```

函数只打印一两行进度；失败时返回非 0。`stage_up` 失败时还会打印最近的
`kubectl get events -A` 和 `kubectl get pods -A -o wide` 末尾几十行。

## 函数

| 函数 | 作用 |
|---|---|
| `cluster_up [配置]` | 删掉已有的 `shorty` 集群，用 `$KIND_NODE_IMAGE` 和指定配置重建；等全部节点 `Ready`。配置里有 `disableDefaultCNI: true` 时只等 apiserver，不等节点（第 24 章装 Calico 前节点本来就是 `NotReady`）。配置默认 `kind-1n.yaml`，可以写 `kind-3n` 或 `kind-3n.yaml`。 |
| `app_images` | 在 lab 的 Docker 里构建 `shorty:0.1`（api）和 `shorty-worker:0.1`（worker）。Dockerfile 在 `app/api`、`app/worker`；重复调用走构建缓存。集群存在时用 `kind load docker-image` 放进节点。 |
| `pull_images 镜像…` | 静默 `docker pull`；集群存在时逐个加载进节点。多平台镜像（postgres、redis 等）走 `_kind_load_image` 的回退路径：`docker save --platform` 后 `kind load image-archive`。 |
| `stage_up <stage> [配置]` | 建集群 → s4 安装 metrics-server 和 Gateway 组件 → `app_images` → s2 起预加载 postgres/redis → `kubectl apply -f stages/<stage>` → 轮询 Deployment、StatefulSet、Job，s4 再等 Gateway 数据面。总时限 `STAGE_WAIT_TIMEOUT`（默认 300 秒）。配置也可以用 `KIND_CONFIG=kind-3n.yaml stage_up s1-api`。 |

`cluster_up` 会删掉同名集群，所以不要先 `cluster_up` 再 `stage_up`，后者会重建一次。章节的
`setup.sh` 直接调 `stage_up` 即可。

## Stage

| stage | 内容 | 从它起步的章 |
|---|---|---|
| `s1-api` | `Namespace shorty`、`ConfigMap shorty-config`、`Deployment api`（1 副本，内存存储）、`Service api`（ClusterIP 8080） | 第 17–20 章 |
| `s2-data` | s1 + `Secret shorty-db`；`Deployment postgres` + `PersistentVolumeClaim postgres-data` + `Service postgres`；`Deployment redis` + `Service redis`；`Job shorty-migrate`；`Deployment worker`；api 2 副本并注入 DB/Redis | 第 21–22 章 |
| `s3-stateful` | s2 的 postgres 换成 `StatefulSet postgres`（1 副本）+ headless `Service postgres` + `volumeClaimTemplates data`，PVC 名为 `data-postgres-0`；其余不变 | 第 23–31 章 |
| `s4-prod` | s3 + 探针、requests/limits、securityContext、`PodDisruptionBudget api`、`HorizontalPodAutoscaler api`（2–5）、`CronJob shorty-cleanup`、`ServiceAccount shorty-api` + `Role/RoleBinding`、`NetworkPolicy`、`Gateway shorty-gw` + `HTTPRoute shorty`、namespace 的 PSA `restricted` 标签 | 第 32–36 章 |

`NetworkPolicy` 是 `default-deny` 加四条 allow：`allow-dns`、`allow-api-db`、`allow-worker-redis`、
`allow-gateway`。由 kindnet 执行，不装 Calico。`allow-gateway` 同时放行 Gateway 数据面到
`nginx-gateway` 控制面的出站，否则默认拒绝下 Gateway 拿不到配置。

s4 的 `Gateway` 数据面是 NGF 在 `shorty` namespace 里创建的 `shorty-gw-nginx` Deployment。它的
Service 是 `LoadBalancer`，kind 里 `EXTERNAL-IP` 一直是 `<pending>`，验证时用：

```bash
kubectl -n shorty port-forward svc/shorty-gw-nginx 8081:80
curl -sS localhost:8081/
```

s5 没有做，因为没有章从它起步。

## 第三方组件（版本写死）

| 组件 | 版本 | 安装方式 |
|---|---|---|
| metrics-server | v0.9.0 | `components.yaml`，再给 Deployment 加 `--kubelet-insecure-tls`（kind 的 kubelet 用自签证书） |
| Gateway API CRD | v1.6.3 | `standard-install.yaml` |
| NGINX Gateway Fabric | 2.8.0 | Helm chart `oci://ghcr.io/nginx/charts/nginx-gateway-fabric`，namespace `nginx-gateway`，控制器 1 副本；数据面镜像 `ghcr.io/nginx/nginx-gateway-fabric/nginx:2.8.0` |

Kubernetes 1.37.0、kind v0.33.0。没有 ingress-nginx（2026-03 已停止维护），也没有 Calico。

## 实测

2026-10-27，`kind-1n.yaml`，每次先用 `lab/reset.sh` 清空，再计时 `stage_up`。内存是
`stage_up` 返回后立刻读 lab 的 `/sys/fs/cgroup/memory.current`，包含 page cache；lab 上限 2.5 GiB。

| stage | 起动时间 | memory.current |
|---|---|---|
| `s1-api` | 36 s | 2.42 GB（2307 MiB） |
| `s2-data` | 56 s | 2.60 GB（2476 MiB） |
| `s3-stateful` | 60 s | 2.60 GB（2478 MiB） |
| `s4-prod` | 129 s | 2.59 GB（2473 MiB） |

s4 的时间主要花在 metrics-server 和 NGF 的镜像下载、启动上。HPA 的 `TARGETS` 在 metrics API
可用前会显示 `<unknown>`，通常再等 10–30 秒；`stage_up` 不等这个，只等工作负载和 Gateway。
3 节点空集群本身约 2.6 GB，跑 s4 时注意 lab 的 2.5 GiB 上限。

## 两个实现细节

- Secret 挂在 `/etc/shorty`，不是 `/run/secrets`。Debian/Alpine 镜像里 `/var/run` 指向 `/run`，
  把 Secret 挂到 `/run/secrets` 会和 kubelet 注入的
  `/var/run/secrets/kubernetes.io/serviceaccount` 撞路径，runc 报
  `create mountpoint ... read-only file system`。
- Docker 29 的 containerd 镜像存储 + kind v0.33 对多平台镜像直接
  `kind load docker-image` 会报 `content digest ... not found`，因为导入时带了 `--all-platforms`。
  `pull_images` 先用 `docker save --platform linux/amd64` 只导出本机平台，再
  `kind load image-archive`，postgres 和 redis 都验证过。
