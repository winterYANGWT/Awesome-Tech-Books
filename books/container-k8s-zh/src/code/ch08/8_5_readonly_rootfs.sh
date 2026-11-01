# 8.5 只读根文件系统 + 显式 tmpfs，叠加非 root、drop ALL、no-new-privileges
docker run -d --name ch08-ro \
    --user 65532:65532 \
    --cap-drop=ALL \
    --security-opt=no-new-privileges \
    --read-only --tmpfs /tmp \
    -p 18082:8080 shorty:dev /out/shorty serve
until curl -fsS localhost:18082/healthz > /dev/null 2>&1; do sleep 1; done
curl -fsS localhost:18082/healthz

# 只读根文件系统上只有 /tmp 这个 tmpfs 可写。
docker exec ch08-ro sh -c 'id; echo ok > /tmp/ch08-write && echo "write /tmp: ok"'
#> fails
#> expect: Read-only file system
docker exec ch08-ro sh -c 'echo no > /ch08-write'

# 把实际生效的选项读回来：rootfs 只读、security-opt、uid。
docker inspect -f 'ReadonlyRootfs={{.HostConfig.ReadonlyRootfs}} SecurityOpt={{json .HostConfig.SecurityOpt}} User={{.Config.User}}' ch08-ro

# 进程本身也没有 capability，且 NoNewPrivs=1。
docker exec ch08-ro grep -E 'CapEff|NoNewPrivs' /proc/self/status

# 去掉 --tmpfs，/tmp 也回到只读的镜像层。
#> fails
#> expect: Read-only file system
docker run --rm --read-only shorty:dev sh -c 'echo x > /tmp/ch08-write'

docker rm -f ch08-ro
