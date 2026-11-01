# 10.1：先看默认 builder，再建一个独立的 docker-container builder
docker buildx ls

# network=host 让 builder 自己和构建容器都能访问 lab 里的本地 registry 和制品库
docker buildx create --name ch10 --driver docker-container --driver-opt network=host --use

# --bootstrap 会拉起 builder 容器并打印它的版本、驱动和平台
#> head: 22
#> expect: Driver:\s+docker-container
#> expect: Status:\s+running
docker buildx inspect --bootstrap ch10

docker buildx ls
