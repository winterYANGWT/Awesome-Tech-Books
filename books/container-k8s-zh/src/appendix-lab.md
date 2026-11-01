# 附录 C 实验环境

本书的 lab 是一台 Linux 主机上的 Docker，里面再起一个特权的 Docker-in-Docker（dind）容器；所有示例都在这第二个 Docker daemon 里跑。这样做的原因很实际：kind 的“节点”本身就是 Docker 容器，示例还会创建和删除容器、网络、卷和整个集群。把这些东西关在一个 lab 容器里，删掉容器就干净，不会在自己的 Docker 里留下镜像、网络和 kind 节点。

嵌套关系是“宿主 Docker → `cbook-lab` 容器 → lab 内的 dockerd → kind 节点容器 → Pod”。lab 容器的 PID 1 是 `docker-init`，不是普通主机上的 `systemd`；看到进程树或 `systemctl` 行为不一样时，先想到这一层。宿主和 lab 都要求 cgroup v2，示例里的 `cpu.max`、`memory.max` 和 `docker info` 的 cgroup 版本都按这个前提写。

不用宿主 daemon 直接跑，是为了避免示例的破坏性操作影响你正在用的容器。`docker build`、`docker run`、`kind create/delete`、`docker network prune` 这些命令在书里出现得很频繁，它们只应该作用在一个可以随时删掉的 daemon 上。

## C.1 镜像里有什么

lab 的镜像定义在 `code/lab/Dockerfile`；书里把这个目录写作 `code/lab/`，在仓库里是 `src/code/lab/`，构建上下文就是它：

```dockerfile
{{#include code/lab/Dockerfile}}
```

基础镜像用固定的 `docker:dind` digest，构建时对应 Docker 29 系列；`KIND_VERSION`、`KUBECTL_VERSION`、`HELM_VERSION` 是构建参数，默认分别拉到 kind v0.33.0、kubectl v1.37.0、Helm v4.3.1。镜像里另装了 bash、coreutils、curl、findutils、git、grep、iproute2、jq、nftables、procps、sed、util-linux（含 util-linux-misc）和 yq，够跑书里的命令、看进程和 cgroup、检查 nftables 规则。

`KIND_NODE_IMAGE` 固定为 `kindest/node:v1.37.0` 的 digest，`cluster_up` 和 `stage_up` 都用它建集群，不会在每次运行时去拉一个浮动的 `latest`。这样书里的 Kubernetes 版本、节点里的 containerd 版本和示例输出才对得上。

`code/lab/daemon.json` 只做一件事：给 Docker Hub 配镜像源。

```json
{{#include code/lab/daemon.json}}
```

Docker Hub 对匿名拉取有限流，频繁重置 lab、反复拉 `postgres`、`redis`、`nginx` 这些镜像时很容易撞上 429。`mirror.gcr.io` 是 Docker Hub 的只读镜像，示例里仍然写 `postgres:18.6-alpine` 这样的正常名字，不需要改镜像引用。如果你的网络访问这个镜像源更快，可以保留；如果不需要，删掉 `daemon.json` 或换成自己的 mirror 再构建即可。它只影响 Docker Hub 的镜像，不影响 `ghcr.io`、`registry.k8s.io` 这些仓库。

| 组件 | 版本 | 说明 |
|---|---|---|
| Docker Engine | 29.9.2 | lab 内的 dockerd，containerd 镜像存储，存储驱动显示为 `overlayfs`，cgroup v2 |
| containerd | 2.3.x | Docker 29 使用的版本；kind 节点里是 2.3.4 |
| kind | v0.33.0 | 建本地集群、加载镜像 |
| kubectl | v1.37.0 | 与服务端 1.37 对齐 |
| Helm | v4.3.1 | 安装 NGINX Gateway Fabric 等组件 |
| kindest/node | v1.37.0 | 所有 kind 集群的节点镜像，digest 固定在 Dockerfile 里 |
| Compose | v5 | 第 16 章实测为 v5.6.1；`docker compose` 插件 |
| metrics-server | v0.9.0 | `s4-prod` 安装，带 `--kubelet-insecure-tls` |
| Gateway API CRD | v1.6.3 | `s4-prod` 安装 |
| NGINX Gateway Fabric | 2.8.0 | `s4-prod` 安装，Helm chart 来自 `ghcr.io/nginx/charts` |

## C.2 启动与进入

在仓库根目录执行下面的命令。构建上下文是 `src/code/lab`，卷 `cbook-lab-docker` 保存 lab 内 dockerd 的镜像和容器数据，`src` 挂到 `/book`，这样容器里的路径就是书里写的 `code/chNN`。

```bash
docker build -t cbook-lab:1 src/code/lab
docker volume create cbook-lab-docker
docker run -d --name cbook-lab --hostname cbook-lab --privileged \
  --memory 2500m --memory-swap 2500m \
  -v cbook-lab-docker:/var/lib/docker \
  -v "$PWD/src:/book" \
  -e DOCKER_TLS_CERTDIR= \
  cbook-lab:1
```

`--privileged` 是 dind 需要的：lab 里的 dockerd 要创建网络命名空间、挂载 overlay、管理 cgroup。`--memory` 和 `--memory-swap` 设成同一个值，是为了让示例里故意制造的 OOM 打在 lab 内，而不是把宿主拖垮；默认 2.5 GB，跑三节点 kind 集群时建议不要再低。

等几秒让内层 dockerd 起来，再进入容器：

```bash
docker exec -it -w /book cbook-lab bash
docker info --format '{{.ServerVersion}} {{.Driver}} cgroup v{{.CgroupVersion}}'
kind version
kubectl version --client
helm version --short
```

如果 `docker info` 报连不上 daemon，先看 `docker logs cbook-lab`；dind 第一次启动要初始化 `/var/lib/docker`，比普通容器慢。内存建议按用途分：只跑 Docker 部分 1.5 GB 够用；跑 Kubernetes 部分时，每个 kind 集群按 2–2.5 GB 准备。三节点空载集群大约 1.2–1.5 GB，跑 `s4-prod` 的 metrics-server、Gateway 和业务对象时会接近 2.5 GB。

不再需要时，删容器和卷：

```bash
docker rm -f cbook-lab
docker volume rm cbook-lab-docker
```

删卷会连同 lab 里的镜像、kind 集群和所有数据一起清掉；只想保留环境时只删容器，下次用同一个卷启动，之前的镜像和集群还在。

## C.3 跑一章

进入容器后，每一章在 `/book/code/chNN` 下操作。先跑该章的 `setup.sh`（如果有），它会把 lab 带到这一章的起点；然后按文件名开头的数字顺序执行示例脚本。示例文件通常叫 `NN_1_说明.sh`、`NN_2_说明.sh`，一个文件做一件事，后面的文件接着前面的状态。

```bash
cd /book/code/ch19
bash setup.sh
ls [0-9]*.sh
bash 19_1_....sh
```

示例脚本开头以 `#>` 开头的注释是 `scripts/check_examples.py` 的标记，例如 `#> fails`、`#> expect:`、`#> hide`、`#> head:`；它们不是 shell 命令，读者直接 `bash` 脚本时可以忽略。`#>` 后面的 `expect` 是验证工具用来固定关键结论的，不影响脚本本身的执行。

同一章的示例共享一个 lab 和一个集群，前一个示例创建的容器、卷、Deployment、Service，后一个示例可以直接用；所以不要跳着跑，也不要在中途删掉前一节留下的对象。每个示例文件会新开一个 bash，shell 变量不跨文件，需要的变量由脚本自己设置。各章都有 `setup.sh`；如果前一章的对象被手动删过，先回到上一章的 `setup.sh` 或对应 stage，不要在半截状态上继续。

各章的起点由 `code/lib/common.sh` 里的函数建立，章节的 `setup.sh` 通常只是 `source ../lib/common.sh` 再调一个 `stage_up`：

| 函数 | 作用 |
|---|---|
| `cluster_up [配置]` | 删掉同名旧集群，用 `$KIND_NODE_IMAGE` 和指定配置重建；默认 `kind-1n.yaml`，也可以传 `kind-3n.yaml` 或禁用默认 CNI 的 `kind-2n-nocni.yaml`；等节点 Ready，配置里禁用默认 CNI 时只等 apiserver |
| `app_images` | 构建 `shorty:0.1` 和 `shorty-worker:0.1`；集群已存在时用 `kind load docker-image` 放进节点 |
| `pull_images 镜像…` | 静默预拉镜像；集群存在时也加载进节点，避免示例输出里混进下载进度 |
| `stage_up <stage> [配置]` | 建集群、按 stage 装前置组件（`s4-prod` 装 metrics-server 和 Gateway）、构建并加载镜像、apply stage 清单、等 Deployment/StatefulSet/Job 就绪；总时限默认 300 秒 |

`stage_up` 会先删掉同名集群再建，所以不要先手动 `cluster_up` 再 `stage_up`，后者会再重建一次。需要换节点数时用 `KIND_CONFIG=kind-3n.yaml stage_up s4-prod`，或直接改章节目录里的 `setup.sh`。

| stage | 内容 | 从它起步的章 |
|---|---|---|
| `s1-api` | `Namespace shorty`、`ConfigMap shorty-config`、单副本 `Deployment api`（内存存储）、`Service api`（ClusterIP 8080） | 第 17–20 章 |
| `s2-data` | s1 加 `Secret shorty-db`、PostgreSQL 和 PVC、Redis、迁移 Job、worker，api 扩到 2 副本 | 第 21–22 章 |
| `s3-stateful` | s2 的 PostgreSQL 换成 `StatefulSet` + headless Service + `volumeClaimTemplates` | 第 23–31 章 |
| `s4-prod` | s3 加探针、requests/limits、securityContext、PDB、HPA、CronJob、ServiceAccount/RBAC、NetworkPolicy、Gateway/HTTPRoute 和 namespace 的 PSA 标签 | 第 32 章及以后 |

`stage_up` 的等待逻辑会轮询 Deployment、StatefulSet 和 Job；`s4-prod` 还会等 Gateway 数据面出现。失败时它打印最近的 Event 和 Pod 状态，先看这些输出，再决定是镜像没加载、PVC 没绑定，还是 Gateway 控制器没起来。`s4-prod` 的 NetworkPolicy 由 kindnet 执行，没有安装 Calico；Gateway 数据面是 NGINX Gateway Fabric 创建的 `shorty-gw-nginx`。

## C.4 贯穿示例 shorty

shorty 的源码和 Compose 文件在 `code/app/`：`api/` 是 Go 服务，`worker/` 是 Python 进程，`compose.yaml` 把 PostgreSQL、Redis、一次性迁移、api、worker 串起来，`secrets/` 放演示用密码文件，`tools/` 是带 `psql` 和 `redis-cli` 的临时容器，`smoke.sh` 跑端到端冒烟测试。第 1–15 章的 Docker 示例大多围绕它展开，第 16 章以后把它搬进 Kubernetes。

不用 kind 也可以单独用 Compose 跑 shorty。在 lab 里进入 `code/app/`，构建并启动：

```bash
cd /book/code/app
docker compose up -d --build
curl -fsS localhost:8080/readyz
curl -sS -X POST localhost:8080/api/links \
  -H 'Content-Type: application/json' \
  -d '{"url":"http://api:8080/","ttl_seconds":3600}'
docker compose down -v
```

`compose.yaml` 里 PostgreSQL 和 Redis 带 healthcheck，`migrate` 等 PostgreSQL 健康后执行，api 和 worker 再等 `migrate` 以 `service_completed_successfully` 结束；这正好演示了 `depends_on` 长格式的用法。数据库密码通过 Compose secret 挂进容器，不写进镜像或普通环境变量。`tools` profile 默认不启动，需要时用 `docker compose --profile tools run --rm tools ...`；`docker compose watch` 按 api 的 `develop.watch` 规则重建。更完整的接口、环境变量和故障开关见 `code/app/README.md`，这里不重复。

## C.5 不用 dind 也可以

如果你的机器本来就是 Linux、cgroup v2，也可以直接在自己的 Docker 上跑。需要自己装好 Docker 29、buildx、kind、kubectl 和 Helm，并把 `KIND_NODE_IMAGE` 设成 `kindest/node:v1.37.0`（最好带 Dockerfile 里的 digest），否则 `cluster_up` 和 `stage_up` 会拒绝建集群。示例仍然从 `code/chNN` 目录运行，`common.sh` 不关心 Docker 是在宿主还是在 dind 里。

这样做的代价是示例的破坏性会直接落到你的 daemon 上：它们会创建和删除容器、网络、卷，创建和删除 kind 集群，修改 cgroup 和 nftables 规则，有的示例还会用 `--privileged`、`--pid=host` 或故意触发 OOM。不要在生产 daemon 上跑，也不要在和别人共用的 Docker 上跑；至少用一个独立的 Docker context、一台一次性虚拟机，或者一个专门跑实验的用户。宿主上的 8080、18080 等端口如果已被占用，先改示例里的端口，不要直接杀掉占用的进程。

没有 dind 时，进程树、`docker-init` 和 cgroup 路径都会和书里不一样：普通主机上 PID 1 是 `systemd`，`/sys/fs/cgroup` 的挂载方式也可能不同。读第 1、3、4、7 章的进程和 cgroup 输出时，把“lab 的 PID 1 是 docker-init”这条差异记在心里。

## C.6 验证环境与已知限制

全书 36 章的示例在 Docker Engine 29.9.2（containerd 2.3.x、cgroup v2、`overlayfs`）、kind v0.33.0、kindest/node v1.37.0（节点内 containerd 2.3.4、Debian 13）、kubectl v1.37.0、Helm v4.3.1 上验证过；记录在 `notes/review.md`。Kubernetes 版本基准是 1.37，Docker 版本基准是 29，Compose 是 v5。

嵌套环境有明确的限制：Pod user namespace（`hostUsers: false`）在“宿主 → lab → kind 节点”的三层嵌套里起不来，挂 `sysfs` 时会报 `operation not permitted`；所以涉及 Pod user namespace 的实验只能讲概念，不能按普通节点上的结果复现。云厂商的 LoadBalancer、节点自动扩缩、托管控制面行为也没有在本地验证，书里只讲概念。kind 自带的 StorageClass 是 `rancher.io/local-path`，默认不支持卷扩容；`s4-prod` 的内存接近 lab 上限，跑之前先确认没有别的任务占着 lab。

版本不同的时候，先看 `kubectl version` 和 `docker version`，再查对应版本的官方文档。书里的输出是在上述版本上生成的，换一个 minor 版本，错误文本、默认值和特性门控都可能变；示例脚本里的 `#>` 断言就是用来发现这类变化的。
