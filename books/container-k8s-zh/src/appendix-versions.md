# 附录 B 版本时间线

这张时间线把全书提到的 Docker 和 Kubernetes 变化按版本排开，用来看清书里的“现状”是哪几年、哪些版本叠出来的。收录标准是正文或 `notes/sources/` 里已经核对过的说法；Kubernetes 特性的 alpha、beta、GA 版本以 `notes/feature-gates.md` 为准，Docker 侧以官方 release notes 为准。年份查不到的就留空，不猜。

Docker 从 17.03 起改用 YY.MM 版本号，所以表里会同时出现 `1.11`、`17.05`、`29.0` 这样的写法；Compose 和 BuildKit 各有自己的版本线，单独列在表末。Kubernetes 从 1.14 到 1.37 基本保持每四个月一个 minor，表里只列书里实际讲到的变化，不追求把每个版本都填满。

## Docker

| 版本 | 年份 | 变化 | 见 |
|---|---|---|---|
| 1.11 | 2016 | 从 Docker 中拆出 containerd 和 runc，容器执行改走 OCI runtime；libcontainer 不再是理解 Docker 的必经概念 | [第 3 章](ch03.md) |
| 1.12 | 2016 | Swarm mode 随 Engine 提供，`docker swarm`、`docker service`、`docker stack` 成为内置编排命令 | [第 14 章](ch14.md) |
| 1.13 | 2017 | `MAINTAINER` 指令弃用，作者信息改用 `LABEL` | [第 9 章](ch09.md)、[第 11 章](ch11.md) |
| 17.05 | 2017 | 引入多阶段构建：`FROM ... AS`、`COPY --from`、`--target` 可以在一个 Dockerfile 里分开发布阶段 | [第 9 章](ch09.md)、[第 10 章](ch10.md) |
| 17.09 | 2017 | `COPY`/`ADD` 支持 `--chown`，复制时直接设置属主，不必再跟一条 `RUN chown -R` | [第 10 章](ch10.md) |
| 18.09 | 2018 | BuildKit 开始随 Engine 提供，可以用 `DOCKER_BUILDKIT=1` 打开；当时还不是默认构建器 | [第 10 章](ch10.md) |
| 19.03 | 2019 | rootless 模式进入 Docker，`dockerd` 可以以非 root 用户运行 | [第 8 章](ch08.md) |
| 20.10 | 2020 | cgroup v2 走出实验阶段，Docker 可以在统一 cgroup 树的节点上运行 | [第 1 章](ch01.md)、[第 7 章](ch07.md) |
| 23.0 | 2023 | Linux 上 Buildx/BuildKit 成为默认构建器，`docker build` 等价于 `docker buildx build`；旧构建器要用 `DOCKER_BUILDKIT=0` | [第 10 章](ch10.md) |
| 25.0 | 2024 | 移除 Device Mapper 存储驱动；传统 graph driver 的默认仍是 `overlay2` | [第 5 章](ch05.md) |
| 29.0 | 2025 | 全新安装默认使用 containerd 镜像存储；nftables 防火墙后端可实验性启用；cgroup v1 弃用；`docker image ls` 改用新的镜像视图 | [第 1 章](ch01.md)、[第 2 章](ch02.md)、[第 5 章](ch05.md)、[第 6 章](ch06.md) |
| 29.6 | 2026 | 在默认 bridge 上创建带 `--link` 的容器时给出弃用告警，计划在 30.0 移除 | [第 6 章](ch06.md) |
| 30.0 |  | 计划移除默认 bridge 上的 link；目前还没有到这一版 | [第 6 章](ch06.md) |
| Compose v1 | 2014 | Python 实现，命令是 `docker-compose`，项目文件顶层要写 `version` 和 2.x/3.x 文件格式 | [第 14 章](ch14.md) |
| Compose v2 | 2020 | 改成 Go 实现，命令变成 `docker compose` 插件；改用 Compose Specification，忽略顶层 `version` 字段 | [第 14 章](ch14.md) |
| Compose v5 | 2025 | 功能与 v2 基本一致，主要增加了官方 Go SDK；今天文档里的当前版本线 | [第 14 章](ch14.md) |
| Buildx 0.15 |  | 开始支持 `docker build --check`，构建前检查 Dockerfile 和构建选项，不执行构建 | [第 13 章](ch13.md) |
| Notary v1 | 2026 | Docker Content Trust 的 Notary v1 服务于 2026-12-08 停止；签名和验签转向 cosign/Sigstore 与准入层策略 | [第 12 章](ch12.md)、[第 13 章](ch13.md) |

## Kubernetes

| 版本 | 年份 | 变化 | 见 |
|---|---|---|---|
| 1.14 | 2019 | kubectl 移除 `-a`/`--show-all`，按状态筛 Pod 改用 field selector，按标签筛用 `-l` | [第 18 章](ch18.md) |
| 1.16 | 2019 | startup 探针进入 alpha；在此之前只有 liveness 和 readiness 两种探针 | [第 19 章](ch19.md) |
| 1.17 | 2019 | `ScheduleDaemonSetPods` 转为 stable；DaemonSet 不再直接写 `spec.nodeName`，改由默认调度器绑定 | [第 20 章](ch20.md) |
| 1.18 | 2020 | kubectl 移除 `--generator`，`kubectl run` 只创建 Pod；startup 探针进入 beta | [第 16 章](ch16.md)、[第 18 章](ch18.md)、[第 19 章](ch19.md) |
| 1.19 | 2020 | 调度框架 stable，扩展点改成 Plugin；ComponentStatus API 弃用；ConfigMap 的 `immutable` 字段引入 | [第 18 章](ch18.md)、[第 27 章](ch27.md)、[第 28 章](ch28.md) |
| 1.20 | 2020 | startup 探针 GA；RuntimeClass GA；apiserver 的 `--insecure-port` 等不安全地址参数从这一版起不再起作用；kubectl 的 `drain` 新增 `--delete-emptydir-data`，旧 `--delete-local-data` 进入弃用 | [第 3 章](ch03.md)、[第 17 章](ch17.md)、[第 19 章](ch19.md)、[第 36 章](ch36.md) |
| 1.21 | 2021 | EndpointSlice 成为 stable；CronJob 的 `batch/v1` GA；ConfigMap 的 `immutable` 成为 stable；`GracefulNodeShutdown` 进入 beta 并默认启用 | [第 21 章](ch21.md)、[第 23 章](ch23.md)、[第 27 章](ch27.md)、[第 36 章](ch36.md) |
| 1.22 | 2021 | 默认用 TokenRequest 把短期 token 投射进 Pod；namespace 自动带 `kubernetes.io/metadata.name` 标签；`extensions/v1beta1` 的 Ingress 不再被接受；Server-Side Apply 成为 stable；`NodeSwap` 进入 alpha | [第 18 章](ch18.md)、[第 19 章](ch19.md)、[第 25 章](ch25.md)、[第 33 章](ch33.md)、[第 36 章](ch36.md) |
| 1.23 | 2021 | gRPC 探针 alpha；`autoscaling/v2` 成为 stable；`kube-scheduler` 的 `--policy-config-file` 等旧参数从这一版起不再支持；`GracefulNodeShutdownBasedOnPodPriority` alpha | [第 19 章](ch19.md)、[第 28 章](ch28.md)、[第 29 章](ch29.md)、[第 36 章](ch36.md) |
| 1.24 | 2022 | 移除 dockershim，节点运行时必须是 CRI 实现；不再自动创建长期 ServiceAccount token Secret；Indexed Job stable；gRPC 探针 beta；`--insecure-port` 移除；`NodeOutOfServiceVolumeDetach` alpha | [第 3 章](ch03.md)、[第 17 章](ch17.md)、[第 19 章](ch19.md)、[第 21 章](ch21.md)、[第 36 章](ch36.md) |
| 1.25 | 2022 | cgroup v2 stable；Pod Security Admission stable，PodSecurityPolicy 移除；CronJob 的 `batch/v1beta1` 不再提供；ephemeral container stable；apiserver 开始提供服务端字段校验 | [第 1 章](ch01.md)、[第 15 章](ch15.md)、[第 16 章](ch16.md)、[第 21 章](ch21.md)、[第 32 章](ch32.md) |
| 1.26 | 2022 | 每个 apiserver 用 `kube-system` 里的 Lease 公布身份；`LegacyServiceAccountTokenNoAutoGeneration` GA；PDB 的 `unhealthyPodEvictionPolicy` alpha；kubelet 要求运行时支持 CRI v1；`NodeOutOfServiceVolumeDetach` beta | [第 1 章](ch01.md)、[第 17 章](ch17.md)、[第 20 章](ch20.md)、[第 30 章](ch30.md)、[第 36 章](ch36.md) |
| 1.27 | 2023 | gRPC 探针 stable；CronJob 的 `timeZone` stable；in-tree 的 AWS EBS、Azure Disk 等驱动移除；原地调整进入 alpha | [第 19 章](ch19.md)、[第 21 章](ch21.md)、[第 26 章](ch26.md)、[第 29 章](ch29.md) |
| 1.28 | 2023 | 原生 sidecar 进入 alpha；GCE PD 的 in-tree 驱动移除；探针级 `terminationGracePeriodSeconds` GA；Pod index 标签进入 beta；`NodeOutOfServiceVolumeDetach` stable；kubelet 支持 `--config-dir` drop-in 目录 | [第 19 章](ch19.md)、[第 21 章](ch21.md)、[第 26 章](ch26.md)、[第 36 章](ch36.md) |
| 1.29 | 2023 | 原生 sidecar 进入 beta 并默认启用；`preStop` 的 `sleep` 动作 alpha；KMS v2 成为 stable、KMS v1 从这一版起默认关闭；kube-proxy 的 nftables 模式 alpha | [第 19 章](ch19.md)、[第 23 章](ch23.md)、[第 27 章](ch27.md)、[第 36 章](ch36.md) |
| 1.30 | 2024 | ValidatingAdmissionPolicy stable；MutatingAdmissionPolicy alpha；`HPAContainerMetrics` stable；`preStop` 的 `sleep` 动作 beta；`procMount: Unmasked` 从这一版起强制要求 `hostUsers: false` | [第 19 章](ch19.md)、[第 29 章](ch29.md)、[第 31 章](ch31.md)、[第 32 章](ch32.md) |
| 1.31 | 2024 | PDB 的 `unhealthyPodEvictionPolicy` stable；CoordinatedLeaderElection alpha；Job 控制器等所有 Pod 终止后才写 `Complete`/`Failed` | [第 17 章](ch17.md)、[第 20 章](ch20.md)、[第 21 章](ch21.md) |
| 1.32 | 2024 | StatefulSet 的 `persistentVolumeClaimRetentionPolicy` stable；CronJob 创建的 Job 带调度时间注解；Pod index 标签 GA；Pod 级资源 alpha | [第 21 章](ch21.md)、[第 22 章](ch22.md)、[第 28 章](ch28.md) |
| 1.33 | 2025 | 原生 sidecar GA；Endpoints 标记弃用；`backoffLimitPerIndex` GA；原地调整进入 beta；CoordinatedLeaderElection beta；kube-proxy 的 nftables 模式 stable；Pod user namespace 的 `idsPerPod` 可配置 | [第 17 章](ch17.md)、[第 19 章](ch19.md)、[第 21 章](ch21.md)、[第 23 章](ch23.md)、[第 29 章](ch29.md)、[第 31 章](ch31.md) |
| 1.34 | 2025 | Dynamic Resource Allocation（DRA）GA；`preStop` 的 `sleep` 动作 GA；Pod 级资源 beta；apiserver 和 kubelet 的分布式追踪 stable；`NodeSwap` stable | [第 19 章](ch19.md)、[第 28 章](ch28.md)、[第 35 章](ch35.md)、[第 36 章](ch36.md) |
| 1.35 | 2025 | 原地调整资源 GA；`JobManagedBy` stable；cgroup v1 弃用；HPA 可配置容差 beta | [第 1 章](ch01.md)、[第 21 章](ch21.md)、[第 29 章](ch29.md) |
| 1.36 | 2026 | MutatingAdmissionPolicy stable；Pod user namespace（`UserNamespacesSupport`）stable | [第 31 章](ch31.md) |
| 1.37 | 2026 | 本书基准；`HPAConfigurableTolerance` stable；Metrics API 成为 stable；kubectl 已没有 `--record` 和 `rolling-update` | [第 20 章](ch20.md)、[第 29 章](ch29.md)、[第 35 章](ch35.md) |

本书的基准是 Kubernetes 1.37（2026-08-26 发布，lab 里是 kindest/node v1.37.0）和 Docker Engine 29（lab 里是 29.9.2，2026-10-20 发布），Compose 按 v5、Helm 按 4.x、kind 按 v0.33 使用。也就是说，书里称为“今天”的行为，大体是 2023 年到 2026 年这几年逐步形成的：BuildKit 默认、containerd 镜像存储、Gateway API、Sidecar、原地调整、DRA 都在这个窗口里落定。

托管集群通常落后几个小版本，读这本书时要先确认服务端版本。`kubectl version` 显示的是客户端和服务端两套版本，书里标为 1.33–1.37 GA 的特性，在落后两三个版本的集群上可能还是 beta 或根本没有对应 API；alpha 和 beta 特性还要看特性门控是否打开。Docker 侧同样如此：托管节点多半直接跑 containerd，Docker 29 的 containerd 镜像存储、nftables 后端这些默认值不一定由读者控制。遇到版本不一致，按“先查 API 是否存在、再查门控、最后查默认值”的顺序读，不要拿书里的版本号硬套。
