# 8.1 默认 root、--user 和卷上的文件属主
# shorty:dev 的镜像没有 USER 指令，所以不带 --user 时就是 uid 0。
#> expect: uid=0\(root\) gid=0\(root\)
docker run --rm shorty:dev id

# 容器里的 root 往命名卷写文件，卷在宿主上的属主就是 0。
docker volume create ch08-userdata
docker run --rm -v ch08-userdata:/data shorty:dev sh -c 'echo from-root > /data/root.txt'
stat -c '%u:%g %a %n' "$(docker volume inspect -f '{{.Mountpoint}}' ch08-userdata)" "$(docker volume inspect -f '{{.Mountpoint}}' ch08-userdata)/root.txt"

# 只改进程 uid：镜像、文件属主都不变。
docker run -d --name ch08-user --user 65532:65532 -p 18081:8080 shorty:dev /out/shorty serve
#> expect: ^65532:65532$
docker inspect -f '{{.Config.User}}' ch08-user
until curl -fsS localhost:18081/healthz > /dev/null 2>&1; do sleep 1; done
curl -fsS localhost:18081/healthz

# 同一个进程：/tmp 是 1777，可以写；根目录属于 root，写不进去。
docker exec ch08-user sh -c 'id; echo ok > /tmp/ch08-write && echo "write /tmp: ok"'
#> fails
#> expect: Permission denied
docker exec ch08-user sh -c 'echo no > /ch08-write'

# 换回 root 看同一个卷，属主还是 0。
docker run --rm --user 0:0 -v ch08-userdata:/data shorty:dev sh -c 'id'
stat -c '%u:%g %a %n' "$(docker volume inspect -f '{{.Mountpoint}}' ch08-userdata)" "$(docker volume inspect -f '{{.Mountpoint}}' ch08-userdata)/root.txt"

docker rm -f ch08-user
docker volume rm ch08-userdata
