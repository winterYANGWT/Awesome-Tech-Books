# 用 exec 形式启动 shorty-api，留给 4.7 继续用；先把优雅退出前的等待调成 2 秒。
docker run -d --name shorty-api -p 8080:8080 -e SHORTY_SHUTDOWN_DELAY=2 shorty:ch04 > /dev/null
#> expect: ok
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors localhost:8080/readyz
#> expect: Name:\s+shorty
docker exec shorty-api head -n 10 /proc/1/status

# exec 进程进入的是容器的 namespace；/proc/1/ns 下的 inode 与 exec 进程自己一致。
docker exec shorty-api ls -l /proc/1/ns
#> expect: pid:\[[0-9]+\]
docker exec shorty-api sh -c 'echo pid1=$(readlink /proc/1/ns/pid) self=$(readlink /proc/self/ns/pid)'

# docker cp 直接拷文件，不需要容器里有 shell 或 tar。
docker cp /etc/hostname shorty-api:/tmp/from-host
docker exec shorty-api cat /tmp/from-host

