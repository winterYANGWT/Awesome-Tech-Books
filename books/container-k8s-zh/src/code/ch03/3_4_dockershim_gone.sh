# 3.4 dockershim 移除之后：Docker 的镜像还能跑，Docker 的命令看不到 Pod
# lab 的 Docker 里有 kind 节点容器；节点里没有 docker 命令。
docker ps --format 'table {{.Names}}\t{{.Image}}\t{{.Status}}'

NODE=$(kind get nodes --name shorty)
docker exec "$NODE" sh -c 'command -v docker || echo "docker: not found"'

# Docker 构建并 load 进节点的镜像，在节点的 CRI 镜像服务里能查到。
docker image ls shorty
docker exec "$NODE" crictl images | grep -E 'IMAGE|shorty'

# kubelet 起的容器只在 CRI 里，Docker 看不见。
pod_id=$(docker exec "$NODE" crictl pods --namespace shorty -q)
docker exec "$NODE" crictl ps --pod "$pod_id"
kubectl get nodes -o wide
