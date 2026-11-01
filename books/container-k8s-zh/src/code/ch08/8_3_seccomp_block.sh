# 8.3 默认 seccomp、拒绝 uname 的 profile、AppArmor 标签
# 默认 profile 生效时，Seccomp 是 2（filter 模式），至少有一个过滤器。
#> expect: Seccomp:\s+2
docker run --rm shorty:dev grep -E 'Seccomp|Seccomp_filters' /proc/self/status

# 自定义 profile：默认放行，只把 uname 改成返回 EPERM。
#> fails
#> expect: Operation not permitted
docker run --rm --security-opt seccomp=seccomp-deny-uname.json shorty:dev uname -a

# 换回默认 profile，同一条 uname 正常。
docker run --rm shorty:dev uname -a

# seccomp=unconfined 把过滤器摘掉，Seccomp 变成 0。
docker run --rm --security-opt seccomp=unconfined shorty:dev grep -E 'Seccomp|Seccomp_filters' /proc/self/status

# daemon 报告的 Security Options：配置了 AppArmor 和 seccomp。
docker info --format '{{json .SecurityOptions}}'

# 但嵌套 lab 里容器进程的标签是 unconfined，不是 docker-default。
docker run --rm shorty:dev cat /proc/self/attr/current
