#!/usr/bin/env bash
# 创建命名卷，看清它由 Docker 管理，不是容器可写层的一部分。
docker volume create shorty-pgdata
docker volume inspect shorty-pgdata --format 'Name={{.Name}} Driver={{.Driver}} Scope={{.Scope}} Mountpoint={{.Mountpoint}}'
docker volume create ch05-scratch > /dev/null
docker volume rm ch05-scratch

# --mount 的写法更显式：type=volume、src/dst 分开写。
docker run -d --name shorty-pg \
    -e POSTGRES_USER=shorty -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD=shorty-demo-password-not-for-production \
    --mount type=volume,src=shorty-pgdata,dst=/var/lib/postgresql \
    postgres:18.6-alpine > /dev/null
for _ in $(seq 1 60); do
    docker exec shorty-pg pg_isready -U shorty -d shorty > /dev/null 2>&1 && break
    sleep 1
done
docker exec shorty-pg pg_isready -U shorty -d shorty

# 在卷里建一张表并写一行，稍后用重建容器来验证它是否还在。
docker exec shorty-pg psql -U shorty -d shorty -v ON_ERROR_STOP=1 -c \
    "CREATE TABLE keep_me (id integer PRIMARY KEY, note text); INSERT INTO keep_me VALUES (1, 'survives'); SELECT * FROM keep_me;"

# 容器删掉，卷仍然在；重建时换回 -v 写法，两种语法等价。
docker rm -f shorty-pg > /dev/null
docker run -d --name shorty-pg \
    -e POSTGRES_USER=shorty -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD=shorty-demo-password-not-for-production \
    -v shorty-pgdata:/var/lib/postgresql \
    postgres:18.6-alpine > /dev/null
for _ in $(seq 1 60); do
    docker exec shorty-pg pg_isready -U shorty -d shorty > /dev/null 2>&1 && break
    sleep 1
done
#> expect: survives
docker exec shorty-pg psql -U shorty -d shorty -c "SELECT * FROM keep_me;"

# 挂载在 inspect 里的样子：Type=volume，Source 是 Docker 管的目录。
docker inspect shorty-pg --format '{{json .Mounts}}'
