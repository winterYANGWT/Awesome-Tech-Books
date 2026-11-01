# 11.1：同一份二进制放进 alpine、python slim、scratch 三种基础镜像
docker build -q --target alpine -f base-compare.Dockerfile -t shorty:alpine ../app/api >/dev/null
docker build -q --target slim -f base-compare.Dockerfile -t shorty:slim ../app/api >/dev/null
docker build -q --target minimal -f base-compare.Dockerfile -t shorty:scratch ../app/api >/dev/null

# 只比较这三个标签，不把起点状态的 shorty:0.1 混进来
for image in shorty:alpine shorty:slim shorty:scratch; do
    docker image inspect --format '{{index .RepoTags 0}} {{.Size}}' "$image"
done | awk '{printf "%-18s %6.1f MB\n", $1, $2/1000000}'

# alpine 和 slim 都带 shell，也各自带着包管理器
#> expect: shell=yes
docker run --rm --entrypoint sh shorty:alpine -c 'echo shell=yes; command -v apk'
#> expect: shell=yes
docker run --rm --entrypoint sh shorty:slim -c 'echo shell=yes; command -v apt-get'

# 两个 libc 不是一回事：alpine 是 musl，slim 是 glibc
docker run --rm --entrypoint sh shorty:alpine -c 'ls /lib/ld-musl-*'
docker run --rm --entrypoint sh shorty:slim -c 'ldd --version | head -1'

# scratch 里没有 shell；--entrypoint 让它直接去找 sh
#> fails
#> expect: exec: "sh": executable file not found
docker run --rm --entrypoint sh shorty:scratch -c 'echo shell=yes'

# 没有 shell 不影响 shorty 自己启动
docker run --rm shorty:scratch version
