# 容器与 Kubernetes 实践

从进程讲到集群：namespace、cgroup 与镜像层，Docker 的运行、构建与分发，Compose，再到 Kubernetes 的工作负载、
网络与存储、配置与弹性、安全、扩展、交付与运维。以 Kubernetes 1.37 和 Docker Engine 29 为基准，全书用同一个
短链服务 shorty 做贯穿示例。9 部分 36 章，另有 3 个附录。

在线阅读：<https://winteryangwt.github.io/Awesome-Tech-Books/container-k8s-zh/>

- `src/`：各章 Markdown，`src/SUMMARY.md` 是目录；`src/code/chNN/` 是各章的示例脚本、清单和由验证工具生成的输出
  （`.out`），`src/code/app/` 是 shorty 的源码，`src/code/lib/`、`src/code/stages/` 是各章起点状态的脚本和清单，
  `src/code/lab/` 是实验环境的镜像定义（见附录 C）。
- 示例都在 Docker-in-Docker 实验环境（Docker Engine 29.9.2、kind v0.33.0、Kubernetes 1.37.0）里实际运行过，
  正文引用的命令输出由工具生成，不是手写的。
- 网页由仓库的 GitHub Actions 用 mdBook 构建，本目录不含生成的 HTML。
- 许可证：文字 CC BY 4.0（见 `LICENSE`），代码 MIT（见 `LICENSE-CODE`）。本目录不适用仓库根目录的 LICENSE。
  部分内容参考和改编自 Kubernetes 文档（CC BY 4.0）与 Docker 文档（Apache-2.0）。本书不是 Kubernetes 项目、
  CNCF 或 Docker 公司的官方出版物；章节主题参考了几本经典的 Docker/Kubernetes 图书，但文字和示例都是重新写的。
- 本书在 AI 辅助下编写，每章经过人工审阅，欢迎通过 issue 指正错误。
