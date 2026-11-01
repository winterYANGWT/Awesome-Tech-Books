# 12.6 离线分发与慢网
# 把镜像存成 tar，删掉本地镜像，再从 tar 装回来；比较两次的镜像 ID。
before=$(docker image inspect --format '{{.Id}}' shorty:0.1)

printf 'before=%s\n' "$before"

docker save -o /tmp/shorty-0.1.tar shorty:0.1
printf 'tar 已生成\n'

docker image rm shorty:0.1

docker load -i /tmp/shorty-0.1.tar

after=$(docker image inspect --format '{{.Id}}' shorty:0.1)
printf 'after=%s\n' "$after"

[ "$before" = "$after" ] && echo 'digest 相同'

docker image ls --format '{{.Repository}}:{{.Tag}}' shorty

docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' shorty:0.1
