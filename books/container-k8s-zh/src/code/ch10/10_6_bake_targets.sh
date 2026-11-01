# 10.6：一个 bake 文件同时构建 api 和 worker；shorty.Dockerfile 与 worker.Dockerfile 分别在两个 target 里
# 第一次不加授权会失败：worker 的 context 在 bake 文件目录之外
#> fails
#> tail: 8
#> expect: additional privileges requested
docker buildx bake --provenance=false -f docker-bake.hcl --load

#> tail: 24
#> timeout: 360
docker buildx bake --allow=fs.read=../app/api --allow=fs.read=../app/worker --provenance=false -f docker-bake.hcl --load

docker image ls shorty
docker image ls shorty-worker

# 本章的终点：api 和 worker 都跑起来
docker network create shorty-net >/dev/null
docker run -d --name shorty-redis --network shorty-net redis:8.8.4-alpine >/dev/null
docker run -d --name shorty-api --network shorty-net -p 18080:8080 shorty:0.1 >/dev/null
docker run -d --name shorty-worker --network shorty-net \
    -e WORKER_REDIS_ADDR=shorty-redis:6379 \
    -e WORKER_DB_URL=postgres://shorty:shorty@postgres:5432/shorty \
    shorty-worker:0.1 >/dev/null

docker ps --filter name=shorty --format '{{.Names}} {{.Image}} {{.State}}'
