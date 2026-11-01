# 8.7 两个安全收紧后的常见故障：绑 80 端口、读挂载的 secret 文件

# 故障一：drop ALL 之后，root 也不能绑 1024 以下端口。
#> fails
#> expect: permission denied
docker run --rm --sysctl net.ipv4.ip_unprivileged_port_start=1024 \
    -e SHORTY_LISTEN=:80 --cap-drop=ALL shorty:dev /out/shorty serve 2>&1 | tail -n 1

# 只把 NET_BIND_SERVICE 加回来；其余 capability 仍然是空。
docker run -d --name ch08-port80 \
    --sysctl net.ipv4.ip_unprivileged_port_start=1024 \
    -e SHORTY_LISTEN=:80 --cap-drop=ALL --cap-add=NET_BIND_SERVICE \
    -p 18083:80 shorty:dev /out/shorty serve
until curl -fsS localhost:18083/healthz > /dev/null 2>&1; do sleep 1; done
curl -fsS localhost:18083/healthz
capsh --decode=$(docker exec ch08-port80 awk '/^CapEff/ {print $2}' /proc/self/status)
docker rm -f ch08-port80

# 故障二：挂进来的 secret 文件是 root:600，uid 65532 读不到。
echo 'demo-password' > /tmp/ch08-secret
chmod 600 /tmp/ch08-secret
stat -c '%u:%g %a %n' /tmp/ch08-secret
#> fails
#> expect: Permission denied
docker run --rm --user 65532:65532 \
    --mount type=bind,src=/tmp/ch08-secret,dst=/run/secrets/db_password,readonly \
    shorty:dev cat /run/secrets/db_password

# 只改文件 mode 就能读；生产里对应 Kubernetes 的 fsGroup 或 initContainer chown。
chmod 644 /tmp/ch08-secret
docker run --rm --user 65532:65532 \
    --mount type=bind,src=/tmp/ch08-secret,dst=/run/secrets/db_password,readonly \
    shorty:dev cat /run/secrets/db_password
rm -f /tmp/ch08-secret
