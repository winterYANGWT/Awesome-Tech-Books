# 11.3：镜像里没有 shell，先看 Entrypoint/Cmd 怎么解释参数
docker image inspect --format 'Entrypoint={{json .Config.Entrypoint}} Cmd={{json .Config.Cmd}}' shorty:0.1

# 直接 append sh，sh 只是变成 /shorty 的参数，shorty 打印用法后退出
#> fails
#> expect: 用法: shorty
docker run --rm shorty:0.1 sh

# 用 --entrypoint 才真的去找 sh；这才是“镜像里没有 shell”的错误
#> fails
#> expect: exec: "sh": executable file not found
docker run --rm --entrypoint sh shorty:0.1 -c 'echo hi'

# 没有 shell 也能把文件取出来：create 不必启动容器
docker create --name shorty-copy shorty:0.1 >/dev/null
docker cp shorty-copy:/shorty /tmp/shorty-copy
ls -l /tmp/shorty-copy
# docker cp 到宿主机后属主是本机用户；镜像里的属主看 export 出来的 tar
docker export shorty-copy | tar -tv shorty
docker rm shorty-copy >/dev/null

# slim 的基础镜像里有 SUID 文件；scratch 里只有二进制、证书和时区
docker run --rm --entrypoint sh shorty:slim -c 'find / -xdev -perm /6000 -type f 2>/dev/null || true'
