# 3.7 排障：Docker 停掉之后 runc 里还剩什么，CRI 里怎么读退出码
# 先看 Docker 管理的容器：运行中时 runc 里有记录。
docker run -d --name ch03-stop shorty:0.1 serve
docker ps --format 'table {{.ID}}\t{{.Names}}'
runc --root /var/run/docker/runtime-runc/moby list

# docker stop 返回后，ch03-stop 的 runc 记录已经没了；停没停要看 Docker 自己的状态。
# --time 1 让它在 1 秒后 SIGKILL，避免 shorty 的优雅退出让退出码在 0 和 137 之间跳。
docker stop --timeout 1 ch03-stop
runc --root /var/run/docker/runtime-runc/moby list
docker ps -a --filter name=ch03-stop --format 'table {{.Names}}\t{{.Status}}'
docker inspect -f '{{.State.Status}} exit={{.State.ExitCode}}' ch03-stop
docker rm ch03-stop

# 换 CRI 这一侧：让一个 Pod 以退出码 7 结束，再从节点上看 ExitCode。
kubectl apply -f pod-exit.yaml
kubectl wait --for=jsonpath='{.status.phase}'=Failed pod/exit-demo -n shorty --timeout=60s
kubectl get pod -n shorty exit-demo -o wide
kubectl get pod -n shorty exit-demo \
  -o jsonpath='{.status.containerStatuses[0].state.terminated.exitCode}{"\n"}'

NODE=$(kind get nodes --name shorty)
docker exec "$NODE" crictl ps -a | grep -E 'CONTAINER|exit-demo'
cid=$(docker exec "$NODE" crictl ps -a | awk '/exit-demo/ {print $1; exit}')
docker exec "$NODE" crictl inspect "$cid" | jq '{state: .status.state, exitCode: .status.exitCode}'

# containerd 本身还在不在，最后看服务和日志。
docker exec "$NODE" systemctl is-active containerd
docker exec "$NODE" journalctl -u containerd -n 5 --no-pager

# 清理本章的 Pod；集群在示例之后删掉，lab 回到“只有镜像、没有容器”的状态。
kubectl delete pod gvisor-demo exit-demo -n shorty --ignore-not-found
#> hide
kind delete cluster --name shorty
