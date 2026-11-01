#!/usr/bin/env bash
# 在容器可写层里写两个文件：一个在根目录，一个在嵌套目录。
docker run -d --name ch05-writable shorty:dev sleep 300 > /dev/null
docker exec ch05-writable sh -c 'echo "saved in container" > /data.txt; mkdir -p /var/lib/shorty; echo cache > /var/lib/shorty/cache.txt'

# docker diff 的三个标记：A 新增、C 修改、D 删除。
#> expect: A /data.txt
#> expect: A /var/lib/shorty/cache.txt
docker diff ch05-writable

# 删掉容器，可写层一起消失；新容器看不到刚才的文件。
docker rm -f ch05-writable > /dev/null
#> fails
#> expect: No such file or directory
docker run --rm shorty:dev sh -c 'ls -l /data.txt'
