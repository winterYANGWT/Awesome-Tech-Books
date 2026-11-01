# 测试进程也在容器里，挂 lab 的 Docker socket，让它去起兄弟容器。
#> timeout: 300
# 依赖版本锁在 testcontainers/go.mod 和 testcontainers/go.sum。
docker run --rm --network host \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v "$PWD/testcontainers:/src:ro" \
  -v ch15-gomod:/go/pkg/mod \
  -w /src \
  golang:1.27.3-trixie \
  go test -mod=readonly -v -run TestRepositoryWithMigrate ./...

# go test 退出后，测试会话创建的容器应该已被清理。
docker ps -a --filter label=org.testcontainers=true --format '{{.Image}}'
