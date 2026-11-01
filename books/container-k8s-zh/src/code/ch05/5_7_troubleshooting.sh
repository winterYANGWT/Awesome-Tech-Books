#!/usr/bin/env bash
# 故障一：宿主目录不存在，-v 自动创建为 root:root；容器按镜像用户 70:70 运行，写不进去。
rm -rf /tmp/ch05-pg-bad
docker run -d --name ch05-pg-bad --user 70:70 \
    -e POSTGRES_USER=shorty -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD=shorty-demo-password-not-for-production \
    -v /tmp/ch05-pg-bad:/var/lib/postgresql postgres:18.6-alpine > /dev/null
for _ in $(seq 1 30); do
    [ "$(docker inspect -f '{{.State.Status}}' ch05-pg-bad)" = exited ] && break
    sleep 1
done

# 定位三件事：日志里的 Permission denied、Mounts 的 Source/Destination、宿主目录的 uid。
#> expect: Permission denied
docker logs ch05-pg-bad 2>&1 | tail -5
docker inspect ch05-pg-bad --format '{{json .Mounts}}'
ls -ldn /tmp/ch05-pg-bad
docker rm -f ch05-pg-bad > /dev/null
rm -rf /tmp/ch05-pg-bad

# 修复：改用命名卷，并确认 shorty-pg 删掉后 shorty-pgdata 还在。
docker rm -f shorty-pg > /dev/null
docker volume ls --filter name=shorty-pgdata --format 'table {{.Driver}}\t{{.Name}}'
docker run -d --name shorty-pg \
    -e POSTGRES_USER=shorty -e POSTGRES_DB=shorty \
    -e POSTGRES_PASSWORD=shorty-demo-password-not-for-production \
    -v shorty-pgdata:/var/lib/postgresql -v shorty-backup:/backup \
    postgres:18.6-alpine > /dev/null
for _ in $(seq 1 60); do
    docker exec shorty-pg pg_isready -U shorty -d shorty > /dev/null 2>&1 && break
    sleep 1
done
docker exec shorty-pg pg_isready -U shorty -d shorty
#> expect: ^\s*3$
docker exec shorty-pg psql -U shorty -d shorty -tAc "SELECT count(*) FROM links;"

# 故障二：PGDATA 指向一个有文件的宿主目录，initdb 拒绝在非空目录上初始化。
rm -rf /tmp/ch05-pg-nonempty
mkdir -p /tmp/ch05-pg-nonempty
echo 'not a database' > /tmp/ch05-pg-nonempty/README
docker run -d --name ch05-pg-nonempty \
    -e POSTGRES_PASSWORD=shorty-demo-password-not-for-production \
    -e PGDATA=/var/lib/postgresql/data \
    -v /tmp/ch05-pg-nonempty:/var/lib/postgresql/data \
    postgres:18.6-alpine > /dev/null
for _ in $(seq 1 30); do
    [ "$(docker inspect -f '{{.State.Status}}' ch05-pg-nonempty)" = exited ] && break
    sleep 1
done
#> expect: exists but is not empty
docker logs ch05-pg-nonempty 2>&1 | tail -6

# 清掉故障容器和临时目录，再把 api 切到 postgres；第 6 章会用网络名替代这个 IP。
docker rm -f ch05-pg-nonempty ch05-ro ch05-tmpfs > /dev/null 2>&1 || true
rm -rf /tmp/ch05-pg-nonempty
PGIP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' shorty-pg)
docker rm -f shorty-api > /dev/null
docker run -d --name shorty-api -p 8080:8080 \
    -e SHORTY_DB_URL="postgres://shorty:shorty-demo-password-not-for-production@$PGIP:5432/shorty?sslmode=disable" \
    shorty:dev /out/shorty serve > /dev/null
for _ in $(seq 1 30); do
    curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
    sleep 1
done
curl -fsS -o /dev/null -w '%{http_code}\n' localhost:8080/readyz
docker ps --filter name=shorty --format 'table {{.Names}}\t{{.Status}}'
docker exec shorty-pg psql -U shorty -d shorty -tAc "SELECT count(*) FROM links;"
