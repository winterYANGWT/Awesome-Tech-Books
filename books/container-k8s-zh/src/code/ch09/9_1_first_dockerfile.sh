# 单阶段：编译器、依赖和源码都留在最终镜像里
docker build --progress=plain --provenance=false --no-cache -f first.Dockerfile -t shorty:dev ../app/api

docker run -d --name shorty-api -p 127.0.0.1:18080:8080 shorty:dev

curl -fsS --retry 30 --retry-delay 1 --retry-all-errors http://127.0.0.1:18080/healthz
