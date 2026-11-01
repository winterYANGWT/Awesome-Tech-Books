# 前言

容器和 Kubernetes 的资料不缺，缺的是一条连贯的路。入门教程大多停在 `docker run` 和一个 Deployment，官方文档按对象和任务分页，想知道“Pod 为什么会被杀”“Service 为什么没有后端”“镜像为什么越修越大”，往往要在几十个页面之间来回跳。更麻烦的是这个领域变得快：几年前的教程里，节点运行时还是 Docker，Pod 安全靠 PodSecurityPolicy，入口流量用 ingress-nginx，Compose 是一个叫 `docker-compose` 的 Python 程序；这些今天都已经被移除、取代或停止维护了。

这本书想做的，是从一个进程讲到一个可以上生产的集群：先拆开容器看清 namespace、cgroup 和镜像层，再用 Docker 运行、构建、分发镜像，然后用 Kubernetes 把同一个应用部署、联网、存储、配置、伸缩、加固，最后讲扩展、交付和运维。全书以 **Kubernetes 1.37** 和 **Docker Engine 29** 为基准，提到的每个特性都写明从哪个版本起可用。

## 一个贯穿全书的应用

从第 4 章起，书里一直用同一个应用做例子：一个叫 shorty 的短链服务。它由 Go 写的 api、Python 写的 worker、PostgreSQL 和 Redis 组成，带健康检查、指标接口和优雅退出，还内置了几个故障开关（变慢、报错、内存泄漏、崩溃、忽略 SIGTERM）。Docker 部分用它讲容器、存储、网络和镜像构建；Kubernetes 部分把它一步步搬进集群，从一个 Deployment 长成带 StatefulSet、Gateway、HPA、NetworkPolicy 和准入策略的完整部署。源码在 `code/app/`，代码量不大，可以从头读完。

## 每一章怎么读

每章从 shorty 当时遇到的一个具体问题开始，然后按“做出来 → 拆开看 → 弄坏它 → 上生产前”的顺序展开：先在实验环境里把东西跑起来，再看底层发生了什么，然后故意制造故障，读症状、定位、修复（每章最后一个编号小节就是排障），最后讲默认值的坑和生产环境的取舍。章末的“速查”列出本章的命令和字段，“官方文档”列出本章依据的官方页面，方便你继续深入。

几个约定：

- **起点**：每章开头的“起点”说明本章开始时环境和 shorty 的状态。运行 `code/chNN/setup.sh` 可以一步到位，所以可以跳着读。
- **旧资料提示**：碰到旧教程里常见、今天已经变了的写法，正文会用一个引用块说明旧写法是什么、现在怎么做、从哪个版本起变的。全书的这些提示汇总在[附录 A](appendix-migration.md)。
- **本书没有在本地验证**：云厂商的负载均衡、节点自动扩缩这类在本地做不了的内容，只讲概念，并会明确写出这句话。

## 示例是怎么验证的

书里所有命令输出都不是手写的。每个示例是一段 shell 脚本，由验证工具在实验环境里逐条执行，把真实输出写成文件，正文直接引用这些文件；关键结论（例如“这里会 OOMKilled”“这里会被准入拒绝”）在脚本里用断言固定下来，环境的行为一变，验证就会失败。故障场景贴的也是真实的错误信息和 Event。

实验环境是一个 Docker-in-Docker 容器：Docker Engine 29.9.2（containerd 镜像存储，cgroup v2），kind v0.33.0，Kubernetes 1.37.0 节点，kubectl 1.37.0，Helm 4.3.1。怎样在自己的机器上搭出同样的环境，见[附录 C](appendix-lab.md)。输出里的容器 ID、Pod 名后缀、IP 和时间每次运行都会不同，你自己跑的时候看到不一样的值是正常的。

## 资料来源

书中的事实、字段、默认值和版本，一律以 Kubernetes 和 Docker 的官方文档为准（本书写作时固定了两套文档的版本：Kubernetes 文档对应 v1.37，Docker 文档取自 2026 年 10 月）；官方文档和实测结果不一致的地方，正文以实测为准，并把两者都写出来。Kubernetes 特性的 alpha、beta、GA 版本取自官方文档里的特性门控数据。

章节的主题和讲述顺序参考了几本经典的 Docker 与 Kubernetes 图书（*Docker in Action*、*The Docker Book*、*Kubernetes: Up and Running*、*The Kubernetes Book*、《Kubernetes 权威指南》），书中的文字和示例都是重新写的，不是这些书的翻译或摘编。

## 许可证与说明

- 正文以 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) 发布，示例代码以 MIT 许可证发布。
- 部分内容参考和改编自 [Kubernetes 文档](https://kubernetes.io/docs/)（CC BY 4.0）和 [Docker 文档](https://docs.docker.com/)（Apache License 2.0），版权归各自的作者所有。
- Kubernetes、Docker 及书中提到的其他名称是各自权利人的商标。本书不是 Kubernetes 项目、CNCF 或 Docker 公司的官方出版物。
- 本书在 AI 辅助下编写：正文和示例由 AI 起草，每章经过人工审阅，技术说法对照官方文档核实，所有示例都在实验环境中实际运行过。难免仍有错误，欢迎指正。
