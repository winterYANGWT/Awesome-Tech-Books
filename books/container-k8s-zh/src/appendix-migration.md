# 附录 A 旧写法迁移表

这张表来自全书 36 章正文里的 108 条 `> **旧资料提示**`。把每章的引用块抽出来，按主题合并同类项，得到一张可以按关键词查的表。同一件事在多章出现的，只占一行，“见”列把相关章节都列出来。

用法是先按主题找到相近的旧写法，再看“现在”一列今天应该怎么写，最后点“见”列的章节链接看完整解释、版本依据和实测输出。表里的“变化版本”只抄正文已经写出的版本；留空不代表没有版本变化，只代表正文没有把版本写死，或者这件事本来就没有一个明确的分界版本。

版本、默认值和字段行为最终以官方文档为准。旧教程里的命令还能不能跑、某个参数从哪个版本起消失，应该用当前版本的 `--help`、`kubectl explain` 和官方 release notes 再确认一次；这张表的作用是帮你快速定位“这里可能变了”，不是替代查证。

## 安装、运行时与容器生命周期

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 把容器说成“轻量虚拟机” | 容器是共享宿主内核的隔离进程；内核漏洞仍可能穿透，强隔离要用虚拟机或 gVisor 这类沙箱 |  | [第 1 章](ch01.md) |
| 限额写在 `/sys/fs/cgroup/cpu`、`/sys/fs/cgroup/memory` 下的 `cpu.shares`、`memory.limit_in_bytes` | cgroup v2 是统一树，写 `cpu.weight`、`cpu.max`、`memory.max` | cgroup v2 在 Kubernetes 1.25 stable；1.35 起 cgroup v1 弃用 | [第 1 章](ch01.md)、[第 7 章](ch07.md) |
| “容器靠 chroot 切根”，chroot 是安全边界 | runc 默认用 pivot_root；chroot 只是历史前身，不是隔离边界 |  | [第 1 章](ch01.md) |
| libcontainer 是“Docker 的容器格式”或“Docker 的原生执行环境” | Docker 1.11 拆出 containerd 和 runc，执行走 OCI runtime；containerd 管生命周期，默认用 runc | Docker 1.11 | [第 3 章](ch03.md) |
| 节点必须装 Docker Engine，kubelet 通过 Docker 管容器 | 节点运行时必须是实现 CRI 的组件（containerd、CRI-O 等）；Docker 要接进来得用 cri-dockerd 适配器 | Kubernetes 1.24 移除 dockershim | [第 3 章](ch03.md)、[第 16 章](ch16.md)、[第 24 章](ch24.md) |
| “Kubernetes 1.25 起才要求 CRI 运行时” | 分界线是 1.24：dockershim 在 1.24 移除，1.23 还能用；1.25 只是移除后的第二个版本 | Kubernetes 1.24 | [第 3 章](ch03.md) |
| 容器只有六种状态，状态机是一条闭环 | 还会出现 `dead`：它不是正常生命周期的一步，而是删除或清理没有完成时留下的状态 |  | [第 4 章](ch04.md) |
| `docker stop` 发 SIGHUP，默认等 30 秒 | 默认信号是 SIGTERM，默认等 10 秒 |  | [第 4 章](ch04.md) |
| 用 `docker attach` 看日志，因为“attach 能看到实时输出” | attach 接的是 PID 1 的 stdio，输入会进 PID 1，Ctrl-C 可能把容器带走；只读跟随日志用 `docker logs -f` |  | [第 4 章](ch04.md) |
| 容器 OOM 时 Docker 不检测、不处理 | `docker inspect .State.OOMKilled` 会标记这次退出是否由 OOM 引起，重启策略按退出行为处理，daemon 还会调整 OOM 优先级 |  | [第 7 章](ch07.md) |
| cgroup v1 的 `cpu.stat` 里有 `throttled_time` | cgroup v2 的字段叫 `throttled_usec`，单位是微秒 |  | [第 7 章](ch07.md) |
| 把 `--privileged` 的 Docker-in-Docker 当成常规做法 | 官方 CI 文档先给托管 CI、官方镜像和 BuildKit，rootless 文档给 `docker:dind-rootless`；privileged 只作为最后手段放在隔离 runner 上 |  | [第 8 章](ch08.md)、[第 15 章](ch15.md) |

## 镜像、构建与 Registry

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 镜像 ID 是 65 个十六进制字符，或者干脆叫“短 UUID” | 镜像 ID 是 64 个十六进制字符（256 bit），`docker image ls` 默认只显示前 12 位；镜像 ID、manifest digest、index digest 不是一回事 |  | [第 2 章](ch02.md) |
| `docker images` 有 `VIRTUAL SIZE` 列，`docker images -a` 会列出构建中间层 | 现代输出是 `SIZE`（经典 store）或 `DISK USAGE`/`CONTENT SIZE`（containerd store）；BuildKit 不创建可启动的中间镜像 |  | [第 2 章](ch02.md) |
| 在最后一层删掉缓存或文档来瘦身，并看镜像体积下降 | 分层文件系统里删除只产生 whiteout，上一层的字节仍然在；要么在同一层里删，要么用多阶段只复制产物 |  | [第 2 章](ch02.md) |
| 把 `docker commit` 当主要构建方式，用“起容器、装软件、commit”代替 Dockerfile | 标准做法是 Dockerfile 加 BuildKit；commit 只用于一次性实验或抢救运行中容器的改动 |  | [第 2 章](ch02.md) |
| “每条 Dockerfile 指令一层” | 只有 `RUN`、`COPY`、`ADD`、`WORKDIR` 这类会改文件系统的指令才产生有内容的层；`ENV`、`EXPOSE`、`USER` 主要改配置 |  | [第 9 章](ch09.md) |
| 旧构建器输出 `Sending build context to Docker daemon`、`Step 1/6`、`Removing intermediate container` | BuildKit 用 `#N` 步骤和 `CACHED`，不再创建中间容器，所以也没有“删除中间容器”这一行 |  | [第 9 章](ch09.md) |
| 用 `MAINTAINER` 写作者信息 | 改用 `LABEL`，新镜像用 OCI 标签 `org.opencontainers.image.*` | Docker 1.13 弃用 `MAINTAINER` | [第 9 章](ch09.md)、[第 11 章](ch11.md) |
| 用 Label Schema 的 `org.label-schema.*` 标注镜像 | 新镜像用 OCI 的 `org.opencontainers.image.*`；旧标签只用于兼容存量镜像 |  | [第 11 章](ch11.md) |
| 用 BuildKit 前先 `export DOCKER_BUILDKIT=1` | Docker Engine 23.0 起 Linux 上默认就是 Buildx/BuildKit；要临时回旧构建器才写 `DOCKER_BUILDKIT=0` | Docker Engine 23.0 | [第 10 章](ch10.md) |
| 多阶段构建是“高级技巧”，生产镜像另维护一份 Dockerfile | 多阶段是 Build 的基础路径；一份 Dockerfile 用 `--target` 同时产出开发和生产镜像 |  | [第 10 章](ch10.md) |
| 先 `COPY` 再 `RUN chown -R` 改属主 | `COPY --chown=65532:65532 源 目标` 直接在复制层改属主，少一层 | Docker 17.09 | [第 10 章](ch10.md) |
| 凭据放在 `~/.dockercfg`，`docker login` 还要输 email | 凭据在 `~/.docker/config.json`，登录只要用户名和密码或 access token |  | [第 12 章](ch12.md) |
| 把 `k8s.gcr.io` 当 Kubernetes 官方镜像仓库 | 用 `registry.k8s.io`；旧 registry 已在 2023 年冻结并重定向 | 2023 | [第 12 章](ch12.md) |

## 网络与服务发现

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| docker0 的网关一定是 `172.17.42.1` | 默认 bridge 通常是 `172.17.0.1/16`，也可能因已有网段或地址池配置变成别的网段；以 `docker network inspect bridge` 的 `IPAM.Config` 为准 |  | [第 6 章](ch06.md) |
| 把 `-p` 叫“NodePort publishing” | Docker 叫端口发布（port publishing）或端口映射；NodePort 是 Kubernetes Service 的概念，两者实现和语义都不同 |  | [第 6 章](ch06.md) |
| 用 `--link db:db` 做容器互联 | 用自定义网络加内嵌 DNS；默认 bridge 上的 link 已弃用 | 29.6 起创建时告警，计划 30.0 移除 | [第 6 章](ch06.md) |
| Pod IP 由 Docker 根据 docker0 分配 | Pod 地址由 CNI 的 IPAM 分配；节点上有没有 docker0 与 Pod 网络无关 | Kubernetes 1.24 移除 dockershim | [第 24 章](ch24.md) |
| Weave Net 是 NetworkPolicy 的常见示例实现 | 上游公司 Weaveworks 已在 2024 年停止运营；新集群优先选 Antrea、Calico、Cilium、Kube-router | 2024 | [第 24 章](ch24.md) |
| CNI 自带 NetworkPolicy，装上就一定生效 | 还要 agent 在每个节点健康、策略在正确的 namespace、选择器选中目标；对象创建成功不代表数据面已编程，要用新连接验证 |  | [第 24 章](ch24.md) |
| kube-proxy 给每个节点分配唯一 IP | 节点 IP 由宿主机或云网络决定，kubelet 可以用 `--node-ip` 指定；Pod IP 由 CNI 分配；kube-proxy 只维护 Service 转发规则 |  | [第 17 章](ch17.md) |
| ClusterIP 一定是 iptables 实现的 | IPVS 和 nftables 模式同样实现 ClusterIP，规则形态完全不同；“IPVS 是推荐模式”也不成立 | IPVS 1.35 弃用，1.43 移除 | [第 23 章](ch23.md) |
| Endpoints 是 Service 的“buddy 对象”，排障只看 `kubectl get endpoints` | 用 EndpointSlice；新脚本用 `kubectl get endpointslice -l kubernetes.io/service-name=<名字>` | EndpointSlice 1.21 stable；Endpoints 1.33 弃用 | [第 23 章](ch23.md) |
| `externalTrafficPolicy: Local` 是“性能优化开关” | 它首先是可用性取舍：保留客户端 IP 的代价是流量只能落在有本地端点的节点上，副本少或分布不均时会制造区域性 503 |  | [第 23 章](ch23.md) |

## 存储与持久化

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 数据卷就是 `/var/lib/docker/volumes` 下的目录，直接进去改 | 用 `docker volume inspect` 管理；直接访问卷数据是 unsupported、undefined behavior，卷驱动或存储后端变化时布局也会变 |  | [第 5 章](ch05.md) |
| 用 `--volumes-from` 共享另一个容器的卷 | 显式命名卷，或在 Compose 里声明卷；`--volumes-from` 把共享关系隐式绑在容器上 |  | [第 5 章](ch05.md) |
| Device Mapper、AUFS 是常见存储驱动 | 传统 graph driver 默认是 `overlay2`；Engine 29.0 起新安装默认用 containerd 镜像存储 | Device Mapper 在 v25.0 移除；containerd 存储 29.0 | [第 5 章](ch05.md) |
| 用 `emptyDir` 存 Cassandra 或数据库数据 | `emptyDir` 跟着 Pod 生命周期，Pod 从节点删除就永久丢失；数据库要用 PVC/CSI 提供的持久卷 |  | [第 22 章](ch22.md) |
| StatefulSet 的 PVC 永远保留 | 默认仍是 Retain，但可以用 `persistentVolumeClaimRetentionPolicy` 把 `whenDeleted` 或 `whenScaled` 设成 Delete | Kubernetes 1.32 | [第 22 章](ch22.md) |
| 用 `storageclass.beta.kubernetes.io/is-default-class` 和 `volume.beta.kubernetes.io/storage-class` | 默认类用 `storageclass.kubernetes.io/is-default-class` 注解，PVC 用 `spec.storageClassName` 字段 |  | [第 26 章](ch26.md) |
| 用 `kubernetes.io/aws-ebs`、`kubernetes.io/gce-pd`、`kubernetes.io/azure-disk` 这些 in-tree provisioner | 装对应的 CSI 驱动，用 CSI 的 provisioner 名 | 在 1.27/1.28 前后移除 | [第 26 章](ch26.md) |

## Compose 与本地开发

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| `compose.yaml` 顶层必须写 `version: "3.7"` 或 `version: "2"` | Compose Specification 把 `version` 变成可选并被忽略；2.x/3.x 文件格式已经合并进规范 | Compose v2 和 v5 | [第 14 章](ch14.md) |
| 用 `docker-compose` 命令（Python 写的 Compose v1） | 用 `docker compose` 插件（Go 实现）；v1 已不在支持列表里 | Compose v2 | [第 14 章](ch14.md) |
| `depends_on` 保证依赖服务已经就绪 | 短格式只保证启动顺序，等价于 `service_started`；等健康检查要长格式 `condition: service_healthy`，等一次性任务成功要 `service_completed_successfully` |  | [第 14 章](ch14.md) |
| 把源码 bind mount 进去，改完 `docker restart` | Compose Watch 把 `sync`、`rebuild`、`restart` 分成明确动作，改代码不一定需要重启容器 |  | [第 15 章](ch15.md) |
| 用 `docker exec` 进容器改文件，再重启进程 | 运行中的容器可写层不是配置来源；临时排查用 `kubectl debug`/ephemeral container，配置改动走 ConfigMap 和新的 rollout |  | [第 15 章](ch15.md) |
| Jenkins 加 privileged Docker-in-Docker 是标准 CI 做法 | 官方 CI 文档先给托管 CI 和官方镜像方案；能挂 socket 或用托管构建服务时，不必默认开 privileged |  | [第 15 章](ch15.md) |
| 用 Play with Kubernetes 在浏览器里起集群 | 这个托管 playground 已停服；本地实验改用 kind、minikube 或 k3d |  | [第 16 章](ch16.md) |
| minikube 必须要 VirtualBox 或 Hyper-V | 现代 minikube 支持多种 driver，包括 Docker 和 Podman；具体矩阵以 minikube 官方文档为准 |  | [第 16 章](ch16.md) |

## 交付、GitOps 与发布

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| Helm 是 Kubernetes 的包管理器，是集群内置能力 | Helm 是集群外的客户端加库，release 记录存在集群的 Secret 里；Kubernetes 官方文档没有 Helm 章节，Kustomize 才是 kubectl 直接支持的那一个 |  | [第 34 章](ch34.md) |

## Kubernetes 工作负载

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| Pod 完全不可变，改任何字段都要删掉重建 | 部分字段可以原地改；改镜像甚至不会重启 Pod，只在下一次容器重启时生效；资源可以通过 `resize` 子资源调整。不可变的是 Pod 身份和大部分 spec |  | [第 19 章](ch19.md) |
| 探针只有 `exec`、`httpGet`、`tcpSocket` 三种机制，类型只有 liveness/readiness | 检查机制有四种，多了 gRPC；类型有三种，多了 startup | startup 1.20 GA；gRPC 1.27 stable | [第 19 章](ch19.md) |
| 更新 Deployment 会删除并重建所有 Pod | 默认 `RollingUpdate` 逐步扩新 ReplicaSet、缩旧 ReplicaSet；只有 `strategy.type: Recreate` 才先删全部旧 Pod |  | [第 20 章](ch20.md) |
| 用 `kubectl get pods -w` 展示滚动更新，或用 `kubectl rolling-update` 做发布 | 用 Deployment 加 `kubectl rollout status`；`kubectl rolling-update` 已经不存在 | kubectl 1.37 | [第 20 章](ch20.md)、[第 34 章](ch34.md) |
| 用 `kubectl ... --record` 自动写 `CHANGE-CAUSE` | 发布前显式 `kubectl annotate deployment/api kubernetes.io/change-cause="..."`，或由 CI 写入这个注解 | kubectl 1.37 已没有这个参数 | [第 20 章](ch20.md)、[第 34 章](ch34.md) |
| DaemonSet 直接写 `spec.nodeName`，不经过 scheduler | DaemonSet 控制器写一条匹配目标节点名的 nodeAffinity，Pod 创建后仍由默认调度器绑定 | 1.12 起 | [第 20 章](ch20.md)、[第 28 章](ch28.md) |
| 用 `kubectl run --restart=OnFailure` 创建 Job | `kubectl run` 现在只创建 Pod；要可重试的一次性任务就写 Job 清单 |  | [第 21 章](ch21.md) |
| 用 `apiVersion: batch/v1beta1` 写 CronJob | 用 `batch/v1`；`batch/v1beta1` 从 1.25 起不再提供 | 1.21 GA；1.25 移除 v1beta1 | [第 21 章](ch21.md) |
| 把数据库迁移写成 api 容器启动脚本里的 `shorty migrate && shorty serve` | 改成独立 Job，api 只负责启动；多副本下每个副本都会各跑一次迁移 |  | [第 21 章](ch21.md) |
| Job 完成后 Pod 会自动删除 | 默认保留完成的 Pod 方便看日志；要自动删就设 `ttlSecondsAfterFinished`，或用 CronJob 的历史保留策略 |  | [第 21 章](ch21.md) |
| 把 StatefulSet 叫 PetSet，或在清单里写 `kind: PetSet` | 只有 `StatefulSet`；`PetSet` 这个资源类型已经不存在 |  | [第 22 章](ch22.md) |

## Service、Ingress 与 Gateway

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| Ingress 写成 `apiVersion: extensions/v1beta1`，后端用 `serviceName`/`servicePort`，不写 `pathType` | 用 `networking.k8s.io/v1`，后端写 `backend.service.name`/`backend.service.port`，每个 path 都写 `pathType` | 1.22 起不再接受旧写法 | [第 25 章](ch25.md) |
| ingress-nginx 是“Kubernetes 默认的 Ingress controller”，安装时直接 apply 它的清单 | 它不是内置组件，而且已在 2025 年 11 月宣布退休、2026 年 3 月起停止维护；新集群不要把它当默认选项 | 2025-11 宣布退休；2026-03 停止维护 | [第 25 章](ch25.md) |
| Gateway API 还在开发，生产先用 Ingress | Gateway API 在 2023 年 10 月发布 v1.0.0，GatewayClass、Gateway、HTTPRoute、GRPCRoute 已 stable；Ingress API 在同一时期冻结 | 2023-10 | [第 25 章](ch25.md) |
| 排查 Ingress 404 时直接翻 nginx 的 `location` 配置 | Gateway API 实现不一定把配置暴露成 nginx 文件；先看 Gateway/HTTPRoute 的 status 条件，再看控制器日志 |  | [第 25 章](ch25.md) |

## 配置、Secret 与安全

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 容器的 root 就是宿主 root，永远如此 | 默认 rootful Docker 确实如此，但 `userns-remap`、rootless Docker 和 Pod user namespace 都会改变映射；判断要看 `docker info` 的 Security Options、uid 映射和 `/etc/subuid` |  | [第 8 章](ch08.md) |
| Docker 默认丢弃 37 个 capability、seccomp 拦截 44 个 syscall | 两个数字都随版本变化；官方只描述“默认集”和“约 44 个、300+ 里”。基线应该写默认集与按需 `drop ALL`，不要写死数字 |  | [第 8 章](ch08.md) |
| 每个 Pod 会自动挂一个 `default-token-*` Secret | 默认是 TokenRequest 投射的短期、自动轮换 token；不再为每个 ServiceAccount 自动创建长期 token Secret | 1.22 起默认投射；1.24 起不再自动创建；1.26 GA | [第 19 章](ch19.md)、[第 27 章](ch27.md)、[第 30 章](ch30.md) |
| 用 `kubectl get secret <sa>-token-*` 找 ServiceAccount token，或 decode 出来写进 kubeconfig | 用 `kubectl create token` 或投射卷；1.24 之后这条默认路径不存在 | 1.24 | [第 27 章](ch27.md) |
| ConfigMap 挂上卷就会自动同步，进程也会立刻读到新值 | 卷里的内容只是“最终”会更新，环境变量不会更新，进程也不会自动重读；要热加载就自己 watch 文件或发信号，否则重启 Pod |  | [第 27 章](ch27.md) |
| 遇到 `CreateContainerConfigError` 就删 Pod 让它重新调度 | 先看 Event 里的消息，再决定补对象、改 key 还是修 RBAC；Secret 名字或 key 真的错了，换多少节点都一样 |  | [第 27 章](ch27.md) |
| PodSecurityPolicy 加 Gatekeeper 是唯一的策略方案；做校验必须装 Gatekeeper/OPA | PSP 已移除；Pod Security Admission 按 namespace 标签执行三档；字段级和对象级校验用 ValidatingAdmissionPolicy 的 CEL 规则；Gatekeeper/OPA 仍适合 Rego 生态和跨集群策略分发 | PSP 1.25 移除；PSA 1.25 stable；VAP 1.30 stable | [第 31 章](ch31.md)、[第 32 章](ch32.md)、[第 33 章](ch33.md) |
| 做 sidecar 注入只能用 mutating webhook，要自己部署服务、管证书、处理超时 | 简单的默认标签、annotation、sidecar 注入可以写成 MutatingAdmissionPolicy 的 CEL；复杂的模板渲染和外部系统查询仍然用 webhook 或控制器 | MAP 1.36 stable | [第 31 章](ch31.md) |
| ABAC 是主要授权模式，维护一行一条 JSON 的策略文件 | RBAC 是标准；官方只在“从 ABAC 升级”一节里讲两者并行，新集群不应该从 ABAC 起步 |  | [第 30 章](ch30.md) |
| 用 `--admission-control` 配置准入插件 | 用 `--enable-admission-plugins` / `--disable-admission-plugins`；旧参数已标为 DEPRECATED |  | [第 30 章](ch30.md) |
| 容器运行时会自动验证镜像签名，Kubernetes 不参与 | 验签发生在准入控制器或准入 webhook；未签名的 Pod 在策略安装前可以正常跑起来 |  | [第 13 章](ch13.md) |

## 扩展 API 与 Operator

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 加一个 CRD 的 YAML 就有了新 API，写个对象就会自动做事 | CRD 只让 apiserver 存取结构化数据；行为要由 custom controller 提供。没有控制器时，CR 创建成功、status 为空、不会产生任何关联资源 |  | [第 33 章](ch33.md) |
| 用 OpenAPI v2 的 `swagger.json` 和 Strategic Merge Patch 描述 API 扩展 | apiserver 同时提供 OpenAPI v2 和 v3，v3 是首选；Server-Side Apply 已经稳定；CRD 不支持 Strategic Merge Patch，更新自定义对象用 JSON Merge Patch、JSON Patch 或 Server-Side Apply | SSA 1.22 stable | [第 33 章](ch33.md) |

## kubectl、API 与集群运维

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| 用 `kubectl get pods -a` 或 `--show-all` 显示所有 Pod | `-a`/`--show-all` 已移除；按状态筛用 `--field-selector=status.phase=Running`，按标签筛用 `-l` | kubectl 1.14 | [第 18 章](ch18.md) |
| 用 `kubectl get componentstatuses` 检查集群健康 | ComponentStatus API 已弃用；用 `kubectl get --raw='/readyz?verbose'` 和 `/livez`，节点和 Pod 看各自的 status 与 Event | 1.19 起弃用 | [第 18 章](ch18.md) |
| 用 `kubectl run --generator=deployment/v1beta1` 创建 Deployment 或 Job | `--generator` 已移除；`kubectl run` 只创建 Pod。要 Deployment 用 `kubectl create deployment`，要 Job 用 `kubectl create job` 或写 YAML | kubectl 1.18 | [第 16 章](ch16.md)、[第 18 章](ch18.md) |
| 把控制平面节点叫“Master 节点” | 官方术语是 control plane（控制平面）；kubeadm 的角色标签是 `node-role.kubernetes.io/control-plane` |  | [第 17 章](ch17.md) |
| apiserver 默认有一个 8080 insecure port，用明文 HTTP 访问 | apiserver 只提供 HTTPS，默认 6443；不安全的地址参数已经移除 | 1.20 起失效；1.24 移除 | [第 17 章](ch17.md)、[第 32 章](ch32.md) |
| 用 `kube-scheduler --policy-config-file` 配 Predicates 和 Priorities | 调度框架已 stable，扩展点改成 Plugin；用 `KubeSchedulerConfiguration` 的 profiles 和 plugins | 1.19 stable；1.23 起不再支持旧参数 | [第 28 章](ch28.md) |
| 证书手工签发一次就不用管 | kubeadm 客户端证书默认 1 年、CA 默认 10 年；不升级控制面就要 `kubeadm certs renew`，kubelet 客户端证书靠 `rotateCertificates` 自动轮换 |  | [第 32 章](ch32.md) |
| 用 `kubectl drain --delete-local-data` 处理使用 emptyDir 的 Pod | 改用 `--delete-emptydir-data`；kubectl 1.37 的 `drain --help` 里已经没有旧参数 | kubectl 1.20 | [第 36 章](ch36.md) |
| 手工搭三节点 etcd，再在前面放 HAProxy/Keepalived，是控制面高可用的常见做法 | 这套做法仍能工作，但负载均衡、健康检查、证书和 etcd 成员都要自己维护；今天更常见 kubeadm 的 `--control-plane-endpoint` 或托管控制面 |  | [第 36 章](ch36.md) |
| 用 Federation v1 / KubeFed 做多集群 | Federation v1 已从 Kubernetes 移除；KubeFed 在 2022 年 11 月宣布、2023 年 4 月归档，推荐 Karmada；今天用 Cluster API、Karmada 这类外部项目，版本和成熟度以各自发布说明为准 | 2022-11 宣布；2023-04 归档 | [第 36 章](ch36.md) |

## 监控、伸缩与供应链

| 旧写法 | 现在 | 变化版本 | 见 |
|---|---|---|---|
| Heapster + InfluxDB + Grafana 是监控标配，Heapster 是 HPA 的指标源 | Heapster 已退役，官方 RBAC 参考里 `system:heapster` 标着 deprecated；资源指标由 metrics-server 提供，长期指标用 Prometheus 或托管服务 |  | [第 29 章](ch29.md)、[第 35 章](ch35.md) |
| 从 `/var/lib/docker/containers` 读容器日志 | Kubernetes 1.24 移除 dockershim 后节点运行时通常是 containerd；集群日志在 `/var/log/pods` 和 `/var/log/containers`，Docker 自己的目录不再是来源 | Kubernetes 1.24 | [第 35 章](ch35.md) |
| `kubectl top` 开箱可用，装不装 metrics-server 都有 | 它要 metrics-server 提供 Metrics API；没装或 APIService 不可用时返回 `Metrics API not available`，不是“节点没有指标” |  | [第 29 章](ch29.md)、[第 35 章](ch35.md) |
| `autoscaling/v1` 的 `targetCPUUtilizationPercentage` 是唯一写法 | `autoscaling/v2` 用 `metrics` 数组表达 CPU、内存、自定义和外部指标，用 `behavior` 控制扩缩节奏，还能按 `ContainerResource` 扩缩 | 1.23 stable | [第 29 章](ch29.md) |
| 镜像扫描放在发布前最后一步，甚至当成可选报告 | `docker build --check`、SBOM 和扫描门禁进主流水线；扫描失败就停止，例外要有负责人和到期时间 |  | [第 11 章](ch11.md)、[第 13 章](ch13.md) |
| 用 Docker Content Trust（`DOCKER_CONTENT_TRUST=1`）给镜像签名 | 用 cosign/Sigstore 签 digest，验签放在准入层；Notary v1 和 DCT 正在退役 | Notary v1 于 2026-12-08 停止 | [第 12 章](ch12.md)、[第 13 章](ch13.md) |
| 扫描只生成报告，扫出什么都继续推 | 把 `--severity HIGH,CRITICAL --exit-code 1` 和构建检查放进主流水线，失败就停止 |  | [第 13 章](ch13.md) |
