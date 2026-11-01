#!/usr/bin/env bash
# 16 MB 的 tmpfs，挂载点和大小都能在容器里看到。
docker run -d --name ch05-tmpfs --tmpfs /run:size=16m,mode=1777 shorty:dev sleep 300 > /dev/null
docker exec ch05-tmpfs df -h /run
docker inspect ch05-tmpfs --format 'Tmpfs={{json .HostConfig.Tmpfs}} Mounts={{json .Mounts}}'

# 写 32 MB，超过 16 MB 上限；dd 会在写满时收到 ENOSPC。
#> fails
#> expect: No space left on device
docker exec ch05-tmpfs dd if=/dev/zero of=/run/big bs=1M count=32

# 清掉半截文件，放一个标记；停止再启动，tmpfs 重新挂载，标记消失。
docker exec ch05-tmpfs sh -c 'rm -f /run/big; echo temp > /run/keep; cat /run/keep'
docker stop ch05-tmpfs > /dev/null
docker start ch05-tmpfs > /dev/null
#> fails
#> expect: No such file or directory
docker exec ch05-tmpfs ls -l /run/keep
