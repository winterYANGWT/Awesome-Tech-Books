# 同一个 tag 先指向 alpine。
docker tag alpine:3.22 ch02-tag:demo
#> expect: linux/amd64
docker image inspect ch02-tag:demo --format 'tag -> {{.Descriptor.digest}} {{.Os}}/{{.Architecture}}'

# 再把同一个 tag 指向 busybox；tag 没变，digest 变了。
docker tag busybox:1.37 ch02-tag:demo
docker image inspect ch02-tag:demo --format 'tag -> {{.Descriptor.digest}} {{.Os}}/{{.Architecture}}'

# 用 alpine 的 digest 启动容器，而不是 tag。
docker run -d --name ch02-pin "alpine@$(docker image inspect alpine:3.22 --format '{{.Descriptor.digest}}')" sleep 300

# 再次覆盖 tag，运行中的容器不受影响，仍然是 alpine。
docker tag busybox:1.37 ch02-tag:demo
docker image inspect ch02-tag:demo --format 'now tag -> {{.Descriptor.digest}}'
docker exec ch02-pin cat /etc/alpine-release
docker inspect ch02-pin --format 'container uses {{.Config.Image}}'

# 清理这个临时容器。
docker rm -f ch02-pin

# 本地构建的镜像也有 RepoDigest；--digests 用另一套列把它显示出来。
#> expect: sha256:
docker image ls --digests shorty

# 直接用这个 digest 运行，不经过 tag。
#> expect: shorty
docker run --rm "shorty@$(docker image inspect shorty:0.1 --format '{{.Descriptor.digest}}')" version
