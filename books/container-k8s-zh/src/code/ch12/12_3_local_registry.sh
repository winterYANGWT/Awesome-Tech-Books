# 12.3 本地 registry 与认证
# 把 12.1/12.2 的临时 registry 换成挂数据卷的固定实例。
docker rm -f shorty-registry >/dev/null 2>&1 || true
docker run -d --name shorty-registry -p 5000:5000 -v registry-data:/var/lib/registry registry:3.0.0 >/dev/null
for _ in $(seq 1 30); do curl -fsS localhost:5000/v2/ >/dev/null 2>&1 && break; sleep 0.5; done

docker inspect --format '{{.State.Status}}' shorty-registry
curl -s localhost:5000/v2/ | jq -c .

docker tag shorty:0.1 localhost:5000/shorty:0.1
docker push localhost:5000/shorty:0.1 2>&1 | tail -2

curl -s localhost:5000/v2/_catalog | jq -c .
curl -s localhost:5000/v2/shorty/tags/list | jq -c .

docker inspect --format '{{range .Mounts}}{{.Type}} {{.Name}} -> {{.Destination}}{{println}}{{end}}' shorty-registry
