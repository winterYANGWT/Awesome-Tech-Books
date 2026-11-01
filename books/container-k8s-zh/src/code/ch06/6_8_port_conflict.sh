# 第二个容器也想发布宿主 8080：这次是端口分配冲突，请求还没到应用就被 Docker 拒绝了。
#> fails
#> expect: port is already allocated
docker run --rm --name shorty-api-dup -p 8080:8080 --network shorty-net \
    shorty:dev /out/shorty serve

# 找占用者：docker ps 的 PORTS 列和宿主 ss 都能看到。
docker ps --filter publish=8080 --format '{{.Names}} {{.Ports}}'
ss -lntp | grep ':8080'

# 换宿主端口即可并存：容器里仍是 8080，宿主换成 8081。
docker run -d --name shorty-api-alt -p 8081:8080 --network shorty-net \
    -e SHORTY_LISTEN=:8080 shorty:dev /out/shorty serve >/dev/null
curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://localhost:8081/healthz
docker rm -f shorty-api-alt >/dev/null
