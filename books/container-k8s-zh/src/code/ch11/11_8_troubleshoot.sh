# 11.7：运行中的 shorty 容器没有 shell，exec 会以 127 退出
docker run -d --name shorty-api -p 18080:8080 shorty:0.1 serve >/dev/null

# 运行中的容器用 exec；Docker 29 在这里返回 127，完整 OCI 文本见 11.3 的 --entrypoint 复现
#> expect: 退出码=127
docker exec shorty-api sh; echo "docker exec 退出码=$?"

# 先确认 Entrypoint/Cmd，再看进程本身是否正常
docker inspect --format 'Entrypoint={{json .Config.Entrypoint}} Cmd={{json .Config.Cmd}}' shorty-api
curl -fsS --retry 30 --retry-all-errors --retry-delay 1 localhost:18080/healthz

# 需要文件时用 docker cp，不必进容器
docker cp shorty-api:/shorty /tmp/shorty-api
ls -l /tmp/shorty-api
