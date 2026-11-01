# bind mount 里的文件属主是 uid 1000、权限 600，容器以别的 uid 运行读不到；
# 把 user 改成属主，或把文件权限放开，才是修法。
rm -rf /tmp/ch15-perm
mkdir -p /tmp/ch15-perm
echo secret > /tmp/ch15-perm/secret.txt
chmod 600 /tmp/ch15-perm/secret.txt
chown 1000:1000 /tmp/ch15-perm/secret.txt
#> fails
#> expect: Permission denied
docker run --rm --user 65534:65534 -v /tmp/ch15-perm:/data:ro \
  busybox:1.37 cat /data/secret.txt

docker run --rm --user 1000:1000 -v /tmp/ch15-perm:/data:ro \
  busybox:1.37 cat /data/secret.txt

# 15.7 的 kind 集群不再需要，先删掉，给最后的编译腾内存。
kind delete cluster --name shorty >/dev/null 2>&1 || true

# DOCKER_HOST 指向不存在的 socket 时，Testcontainers 在启动容器之前就报错。
#> fails
#> timeout: 300
#> expect: start container with DOCKER_HOST=unix:///tmp/ch15-no-such.sock
docker run --rm \
  -e DOCKER_HOST=unix:///tmp/ch15-no-such.sock \
  -e TESTCONTAINERS_RYUK_DISABLED=true \
  -v "$PWD/testcontainers:/src:ro" \
  -v ch15-gomod:/go/pkg/mod \
  -w /src \
  golang:1.27.3-trixie \
  go test -mod=readonly -run TestDockerEnvironment ./...
