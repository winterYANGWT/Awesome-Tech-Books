#!/usr/bin/env bash
# 先看数据目录和运行用户：uid 不是背下来的，读容器里的 /etc/passwd。
docker exec shorty-pg sh -c 'id postgres; echo "PGDATA=$PGDATA"; grep "^postgres:" /etc/passwd; ls -ld "$PGDATA"'

# 用 shorty 自己的 migrate 建 links 表；--network container 让 127.0.0.1 指向 postgres。
docker run --rm --network container:shorty-pg \
    -e SHORTY_DB_URL='postgres://shorty:shorty-demo-password-not-for-production@127.0.0.1:5432/shorty?sslmode=disable' \
    shorty:0.1 migrate

# 插入三行，作为备份前的基准。
docker exec shorty-pg psql -U shorty -d shorty -v ON_ERROR_STOP=1 -c \
    "INSERT INTO links (code, url) VALUES ('abc123', 'https://example.com/a'), ('def456', 'https://example.com/b'), ('ghi789', 'https://example.com/c') ON CONFLICT (code) DO NOTHING; SELECT count(*) FROM links;"

# 重建容器，把 pgdata 和 backup 两个卷都挂上；备份写到 /backup，不留在可写层。
docker volume create shorty-backup
docker rm -f shorty-pg > /dev/null
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

# -Fc 是 custom 格式，文件头是 PGDMP；docker exec 走容器内的 Unix socket。
docker exec shorty-pg pg_dump -U shorty -d shorty -Fc -f /backup/shorty.dump
docker exec shorty-pg sh -c 'ls -lh /backup/shorty.dump; head -c 5 /backup/shorty.dump; echo'

# 模拟数据丢失：删表后查询会报 relation does not exist。
docker exec shorty-pg psql -U shorty -d shorty -c "DROP TABLE links, keep_me;"
#> fails
#> expect: does not exist
docker exec shorty-pg psql -U shorty -d shorty -c "SELECT count(*) FROM links;"

# 从 custom 格式恢复，再数一次行数。
docker exec shorty-pg pg_restore -U shorty -d shorty /backup/shorty.dump
#> expect: ^\s*3$
docker exec shorty-pg psql -U shorty -d shorty -tAc "SELECT count(*) FROM links;"
