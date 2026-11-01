# 14.5：默认只起五个服务，tools 要显式启用 profile
docker compose ps -a --services | sort

# config 能列出 profile 里的服务，但 up 不会起它
docker compose --profile tools config --services | sort

#> tail: 8
docker compose --profile tools up -d

# 现在多了一个 tools；migrate 是一次性服务，所以加 -a 才看得到
docker compose --profile tools ps -a --services | sort

# 用 tools 里的 psql 查数据库，--rm 让一次性容器跑完就删
docker compose --profile tools run --rm tools sh -c 'PGPASSWORD=$(cat /run/secrets/db_password) psql -h postgres -U shorty -d shorty -tAc "select count(*) from links"'

# 多文件覆盖：后一个文件里的 environment 键覆盖前一个，其他键保留
docker compose -f compose.yaml -f compose.dev.yaml config --format json | jq '.services.api.environment | {SHORTY_LOG_LEVEL, SHORTY_BASE_URL, SHORTY_LISTEN}'

# 把 tools 停掉，恢复“五个服务、tools 未启动”的状态
docker compose --profile tools rm -sf tools
docker compose ps -a --services | sort
