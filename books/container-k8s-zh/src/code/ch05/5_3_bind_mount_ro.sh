#!/usr/bin/env bash
# --mount 默认不创建宿主路径，源目录不存在时直接拒绝启动。
#> fails
#> expect: bind source path does not exist
docker run --rm --mount type=bind,src="$PWD/no-such-config",dst=/etc/shorty shorty:dev true

# -v 会自动创建一个 root 拥有的目录，这是两种写法的一个实际差别。
rm -rf /tmp/ch05-autocreate
docker run --rm -v /tmp/ch05-autocreate:/data shorty:dev sh -c 'ls -ld /data'
ls -ldn /tmp/ch05-autocreate
rmdir /tmp/ch05-autocreate

# 把配置目录只读挂进容器：源是宿主路径，不是卷名。
docker run -d --name ch05-ro -v "$PWD/config:/etc/shorty:ro" shorty:dev sleep 300 > /dev/null
docker exec ch05-ro cat /etc/shorty/app.env
docker inspect ch05-ro --format '{{json .Mounts}}'

# 容器里的进程能看到文件，但写不进去；错误来自挂载的只读属性。
#> fails
#> expect: Read-only file system
docker exec ch05-ro sh -c 'echo "change" >> /etc/shorty/app.env'
