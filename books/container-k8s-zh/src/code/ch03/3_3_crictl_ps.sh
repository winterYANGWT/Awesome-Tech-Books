# 3.3 从节点内部看 Pod：kubelet 通过 CRI 把工作交给 containerd
# 建一个单节点集群，并把镜像直接 load 进节点，避免把拉取进度混进输出。
#> hide
#> timeout: 600
kind create cluster --name shorty --wait 120s

#> hide
kind load docker-image shorty:0.1 --name shorty

# 给节点里的 crictl 一个默认 endpoint，后面的命令就不用每次带 flag。
#> hide
NODE=$(kind get nodes --name shorty)
#> hide
docker exec "$NODE" sh -c 'printf "runtime-endpoint: unix:///run/containerd/containerd.sock\nimage-endpoint: unix:///run/containerd/containerd.sock\ntimeout: 10\n" > /etc/crictl.yaml'
#> hide
docker exec "$NODE" crictl pull busybox:1.37

kubectl create namespace shorty
kubectl apply -f shorty-deployment.yaml
kubectl rollout status deployment/shorty -n shorty --timeout=120s

# kubectl 看到的是 API server 里的 Pod；crictl 看到的是节点上真实的 sandbox 和容器。
kubectl get pods -n shorty -o wide
NODE=$(kind get nodes --name shorty)
docker exec "$NODE" crictl pods --namespace shorty
pod_id=$(docker exec "$NODE" crictl pods --namespace shorty -q)
docker exec "$NODE" crictl ps --pod "$pod_id"

# Pod 的 sandbox 就是 pause 容器：它先起来，业务容器再加入它的 namespace。
docker exec "$NODE" crictl images | grep -E 'IMAGE|pause'
docker exec "$NODE" crictl inspectp "$pod_id" \
  | jq '{id: .status.id, state: .status.state, metadata: .status.metadata,
         runtimeHandler: .status.runtimeHandler}'

# 节点上的 containerd 是独立进程，每个 sandbox/容器对应一个 shim。
docker exec "$NODE" ps -eo pid,ppid,comm | grep -E 'containerd|kubelet' | sed -n '1,12p'
