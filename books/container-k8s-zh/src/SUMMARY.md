# Summary

[前言](index.md)

# 第一部分 容器是什么

- [第 1 章 从进程到容器：namespace、cgroup、rootfs（手工用 `unshare` 做一个容器）](ch01.md)
- [第 2 章 镜像：层、内容寻址与 OCI 格式（containerd 镜像存储）](ch02.md)
- [第 3 章 运行时栈：CLI → dockerd → containerd → runc，以及 CRI](ch03.md)

# 第二部分 用 Docker 运行容器

- [第 4 章 容器的生命周期与 PID 1：信号、重启策略、日志、exec](ch04.md)
- [第 5 章 数据：volume、bind mount、tmpfs，备份与权限](ch05.md)
- [第 6 章 单机网络：bridge、内置 DNS、端口发布与 nftables，link 为什么淘汰](ch06.md)
- [第 7 章 资源限制与 cgroup v2：CPU、内存、OOM](ch07.md)
- [第 8 章 容器安全基线：用户、capability、seccomp、userns、rootless](ch08.md)

# 第三部分 构建与分发镜像

- [第 9 章 Dockerfile 与构建缓存](ch09.md)
- [第 10 章 BuildKit：多阶段、缓存挂载、secret、多架构](ch10.md)
- [第 11 章 小而安全的镜像：基础镜像选择、非 root、漏洞扫描](ch11.md)
- [第 12 章 Registry、tag 与 digest、私有仓库与离线分发（含国内网络下取镜像）](ch12.md)
- [第 13 章 软件供应链：SBOM、provenance、签名与验签](ch13.md)

# 第四部分 多容器应用与开发流程

- [第 14 章 Compose：服务、网络、卷、依赖与健康检查、profiles、watch（附：Swarm 去哪了）](ch14.md)
- [第 15 章 开发与 CI 中的容器：dev container、Testcontainers、CI 里构建镜像](ch15.md)

# 第五部分 Kubernetes 核心

- [第 16 章 从 Compose 到 Kubernetes：声明式 API、控制循环，用 kind 起集群](ch16.md)
- [第 17 章 集群架构：一次 `kubectl apply` 背后发生了什么](ch17.md)
- [第 18 章 API 对象与 kubectl：GVK、label、namespace、server-side apply](ch18.md)
- [第 19 章 Pod：多容器、init、原生 sidecar（1.33 GA）、探针、优雅终止](ch19.md)
- [第 20 章 Deployment 与滚动更新、DaemonSet](ch20.md)
- [第 21 章 Job 与 CronJob](ch21.md)
- [第 22 章 StatefulSet 与有状态应用](ch22.md)

# 第六部分 网络与存储

- [第 23 章 Service、EndpointSlice、kube-proxy 与集群 DNS](ch23.md)
- [第 24 章 Pod 网络、CNI 与 NetworkPolicy](ch24.md)
- [第 25 章 入口流量：从 Ingress 到 Gateway API（ingress-nginx 已于 2026-03 停止维护）](ch25.md)
- [第 26 章 存储：PV、PVC、StorageClass、CSI、快照](ch26.md)

# 第七部分 配置、调度与弹性

- [第 27 章 ConfigMap 与 Secret，以及外部 secret 管理](ch27.md)
- [第 28 章 资源管理与调度：requests/limits、QoS、亲和、taint、优先级，以及 GPU 与 DRA（1.34 GA）](ch28.md)
- [第 29 章 自动伸缩：HPA、VPA、原地调整资源（1.35 GA）、节点伸缩](ch29.md)

# 第八部分 安全

- [第 30 章 认证、授权与 RBAC、ServiceAccount](ch30.md)
- [第 31 章 工作负载安全与准入控制：securityContext、Pod Security Admission、ValidatingAdmissionPolicy](ch31.md)
- [第 32 章 威胁建模与集群加固](ch32.md)

# 第九部分 扩展、交付与运维

- [第 33 章 扩展 Kubernetes：CRD、控制器与 Operator](ch33.md)
- [第 34 章 打包与交付：Kustomize、Helm、GitOps、渐进式发布](ch34.md)
- [第 35 章 可观测性与排障：日志、指标、追踪，排障顺序](ch35.md)
- [第 36 章 集群运维：升级、etcd 备份恢复、节点维护、多集群](ch36.md)

# 附录

- [附录 A 旧写法迁移表](appendix-migration.md)
- [附录 B 版本时间线](appendix-versions.md)
- [附录 C 实验环境](appendix-lab.md)
