# 12.1 tag、digest 与 repository
# 先给本地镜像加上 registry 里的名字，推上去，再从 RepoDigests 里取摘要。
docker tag shorty:0.1 localhost:5000/shorty:0.1
docker push localhost:5000/shorty:0.1 2>&1 | tail -2

docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' localhost:5000/shorty:0.1

# 取 localhost:5000/shorty 这个名字下的摘要；同一镜像的短名字通常也带同一条摘要。
digest=$(docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' localhost:5000/shorty:0.1 | awk -F@ 'NF == 2 {print $2; exit}')

#> expect: ^digest=sha256:[0-9a-f]{64}$
printf 'digest=%s\n' "$digest"

#> expect: ^shorty dev$
docker run --rm "localhost:5000/shorty@$digest" version

# 用 0.2 的内容覆盖 0.1 这个 tag：tag 动了，上面那个摘要不会动。
#> tail: 1
docker build -q --build-arg VERSION=0.2 -t localhost:5000/shorty:0.1 ../app/api

docker push localhost:5000/shorty:0.1 2>&1 | tail -2

#> expect: ^shorty dev$
docker run --rm "localhost:5000/shorty@$digest" version

#> expect: ^shorty 0.2$
docker run --rm localhost:5000/shorty:0.1 version
