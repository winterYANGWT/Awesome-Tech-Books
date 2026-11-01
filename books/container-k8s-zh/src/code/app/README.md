# shorty：全书的示例应用

一个小型短链服务。`api` 接收链接、生成短码并把点击重定向到原地址；`worker` 从 Redis 队列取任务，
抓取目标页面的 `<title>` 写回 PostgreSQL。它有健康检查、Prometheus 指标和一组故障开关，
供 Docker 与 Kubernetes 章节演示探针、滚动更新、OOM、CrashLoopBackOff 和优雅退出。

## 组成

| 目录 / 服务 | 内容 |
|---|---|
| `api/` | Go 单二进制 `shorty`：`serve`、`migrate`、`cleanup`、`version` |
| `worker/` | Python 进程：`BRPOP shorty:jobs`，抓标题，写数据库 |
| `compose.yaml` | PostgreSQL、Redis、一次性 migrate、api、worker，以及 `tools` profile |
| `tools/` | 带 `psql` 和 `redis-cli` 的临时工具容器 |
| `secrets/` | 仅用于演示的数据库密码文件 |
| `smoke.sh` | 端到端冒烟测试，一条命令跑完再清理 |

## 快速开始

在 lab 里执行（lab 的当前目录就是项目根目录）：

```bash
docker compose up -d --build
curl -fsS localhost:8080/readyz
curl -sS -X POST localhost:8080/api/links \
  -H 'Content-Type: application/json' \
  -d '{"url":"http://api:8080/","ttl_seconds":3600}'
curl -sS localhost:8080/api/links/<code>
docker compose down -v
```

创建短链时把目标写成 `http://api:8080/`，worker 从容器网络里能访问到它，标题会变成 `Shorty`。
`ttl_seconds` 省略或为 0 表示不过期。`short_url` 用 `SHORTY_BASE_URL` 拼出，默认是 `http://localhost:8080`。

一键验证：

```bash
bash src/code/app/smoke.sh   # 在 lab 内执行
```

## 接口

| 方法 | 路径 | 说明 |
|---|---|---|
| GET | `/` | 极简 HTML 页面，`<title>Shorty</title>`，带创建表单 |
| POST | `/api/links` | 请求 `{"url":"...","ttl_seconds":3600}`，返回 201 和 `{"code","short_url"}` |
| GET | `/r/{code}` | 302 跳转，点击数 +1；不存在或过期返回 404 |
| GET | `/api/links/{code}` | 返回 `url`、`clicks`、`title`、`created_at`、`expires_at` |
| GET | `/healthz` | 进程活着就返回 200（liveness） |
| GET | `/readyz` | 数据库/Redis 可用且未在退出流程中返回 200，否则 503 并说明原因 |
| GET | `/metrics` | Prometheus 文本格式 |
| GET / POST | `/admin/fault` | 查看/修改故障开关，参数用查询字符串 |

`/metrics` 包括按路由和状态码的请求数、请求耗时直方图、正在处理的请求数、进程启动时间；
内存模式下还有 `shorty_memory_links`。

## 环境变量

api：

| 变量 | 默认值 | 说明 |
|---|---|---|
| `SHORTY_LISTEN` | `:8080` | 监听地址 |
| `SHORTY_DB_URL` | 空 | 空则用内存存储，启动日志会明确提示数据不持久 |
| `SHORTY_DB_PASSWORD_FILE` | 空 | 从文件读密码，写入 `SHORTY_DB_URL`（不打印密码） |
| `SHORTY_REDIS_ADDR` | 空 | 配置后每个新链接 `LPUSH shorty:jobs` |
| `SHORTY_BASE_URL` | `http://localhost:8080` | 生成 `short_url` |
| `SHORTY_SHUTDOWN_DELAY` | `5` | 收到 SIGTERM/SIGINT 后先等几秒再关 HTTP |
| `SHORTY_LOG_LEVEL` | `info` | `debug`、`info`、`warn`、`error` |

worker：

| 变量 | 默认值 | 说明 |
|---|---|---|
| `WORKER_REDIS_ADDR` | 必填 | Redis 地址，也可写 `redis://...` |
| `WORKER_DB_URL` | 必填 | PostgreSQL DSN |
| `WORKER_DB_PASSWORD_FILE` | 空 | 从文件读密码并填入 DSN |
| `WORKER_CONCURRENCY` | `1` | 并发 worker 线程数 |
| `WORKER_FAULT_SLOW_MS` | `0` | 每个任务额外 sleep 的毫秒数，用来制造队列积压 |
| `WORKER_HEARTBEAT_FILE` | `/tmp/worker-heartbeat` | 每轮更新一次心跳文件，供 exec liveness 探针使用 |

## 故障开关

环境变量和 `/admin/fault` 使用同一组参数；`/admin/fault` 本身不受故障影响，避免打开 `error_rate=1`
后关不掉。

| 参数 | 作用 |
|---|---|
| `latency_ms=500` | 每个请求额外延迟 500 ms |
| `error_rate=0.5` | 按比例返回 500，范围 0–1 |
| `leak_mb=8` | 每个请求分配 8 MB 并保留，演示 OOMKilled |
| `not_ready=true` | `/readyz` 一直 503 |
| `crash_after=10` | 从进程启动算起 10 秒后以非 0 退出；运行中修改也生效，改小可能立即触发 |
| `ignore_sigterm=true` | 忽略 SIGTERM，演示优雅终止超时后被 SIGKILL |

```bash
curl -X POST 'localhost:8080/admin/fault?latency_ms=500&not_ready=true'
curl localhost:8080/admin/fault
curl -X POST 'localhost:8080/admin/fault?latency_ms=0&not_ready=false'
```

api 启动时数据库或 Redis 连不上不会退出：进程照常监听，后台每 2 秒重试，连通前 `/readyz` 返回 503。
收到 SIGTERM/SIGINT 后先把 readiness 置为失败，等待 `SHORTY_SHUTDOWN_DELAY` 秒，再调用
`http.Server.Shutdown`（上限 10 秒），每一步都写 JSON 日志。

## 镜像与测试

基础镜像：`golang:1.27.3-trixie`、`gcr.io/distroless/static-debian13:nonroot`、
`python:3.15.2-slim-trixie`、`postgres:18.6-alpine`、`redis:8.8.4-alpine`。
`tools` 复用 `postgres:18.6-alpine`，只额外装一个 `redis` 包。
api 静态编译后放进 distroless；worker 先装依赖再复制代码，以数字 UID 运行。

```bash
# Go
docker run --rm -v "$PWD/api:/src" -w /src golang:1.27.3-trixie go test ./...
# Python
docker run --rm -v "$PWD/worker:/app" -w /app python:3.15.2-slim-trixie \
  sh -c 'pip install -q -r requirements.txt && python -m unittest'
```

## Compose 细节

- `postgres` 和 `redis` 有 healthcheck；`migrate` 等 postgres 健康，`api`/`worker` 等 `migrate`
  以 `service_completed_successfully` 结束。
- PostgreSQL 18 的 `PGDATA` 是 `/var/lib/postgresql/18/docker`，所以命名卷挂在 `/var/lib/postgresql`。
- 数据库密码走 Compose secret `/run/secrets/db_password`，不写进镜像或普通环境变量。
  它是以文件形式挂载的，文件权限要让容器里的非 root 用户可读（示例文件为 644）。
- `tools` profile 默认不启动：
  ```bash
  docker compose --profile tools run --rm tools \
    sh -c 'PGPASSWORD=$(cat /run/secrets/db_password) psql -h postgres -U shorty shorty'
  docker compose --profile tools run --rm tools redis-cli -h redis
  ```
- `docker compose watch` 使用 api 的 `develop.watch`，代码变化时 rebuild。

`secrets/db_password.txt` 和这里的所有凭据都只用于本地演示。
