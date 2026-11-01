# 16.3 删掉 setup 建的集群，从零看一遍 kind 创建单节点集群。
kind delete cluster --name shorty
kind create cluster --name shorty --image "$KIND_NODE_IMAGE" --wait 120s
kind export kubeconfig --name shorty
kubectl config current-context
kubectl cluster-info
kubectl get nodes -o wide

# 节点本身是一个 Docker 容器；节点里的容器运行时是 containerd，不是 Docker。
NODE=$(kind get nodes --name shorty)
echo "节点容器：$NODE"
docker inspect --format '{{.Config.Image}}' "$NODE"
docker exec "$NODE" systemctl is-active containerd kubelet
docker exec "$NODE" crictl info | jq -r '.config.containerd.defaultRuntimeName, .config.containerd.runtimes.runc.runtimeType'
if docker exec "$NODE" test -S /var/run/docker.sock; then
  echo "节点里有 Docker socket"
else
  echo "节点里没有 Docker daemon socket"
fi

# MemUsage 会随 page cache 和启动时间变化，这里只看量级。
docker stats --no-stream --format '{{.Name}}  {{.MemUsage}}' "$NODE"
