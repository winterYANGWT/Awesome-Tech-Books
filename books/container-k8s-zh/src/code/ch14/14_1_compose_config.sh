# 14.1：先读合并后的模型，不启动任何容器
# 默认不启用任何 profile，所以 tools 不在这里
docker compose config --services

# 顶层没有 version 字段；name 是项目名，volumes 里是逻辑名 pgdata
docker compose config --format json | jq '{name, has_version: has("version"), services: (.services | keys), volumes: (.volumes | keys)}'

# 变量有默认值：不设置时发布 8080，设置后覆盖
SHORTY_API_PORT=18080 docker compose config --format json | jq '.services.api.ports[0] | {published, target}'

# 卷的逻辑名不等于 Docker 里的名字；实际名字在 up 之后看
docker compose config --volumes
