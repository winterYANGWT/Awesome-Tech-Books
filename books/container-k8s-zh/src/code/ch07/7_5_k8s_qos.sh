# 本章唯一的 Kubernetes 实验：单节点 kind 集群，跑完就删。
source ../lib/common.sh

# kind 节点和 Docker 栈同时跑会顶到 1.5 GB 的 lab 上限；先把栈停掉，
# 数据在卷里，7.5 结束后再 start 回来。
#> hide
{
    docker stop shorty-api shorty-pg shorty-redis > /dev/null 2>&1 || true
    sync
    echo 300M > /sys/fs/cgroup/memory.reclaim 2>/dev/null || true
}

# kind 节点偶尔启动超时；失败时 cluster_up 会先删掉半成品集群，再重试。
#> timeout: 600
ok=0
for _ in 1 2; do
    if cluster_up > /tmp/ch07-cluster.log 2>&1; then
        ok=1
        break
    fi
    sleep 2
done
if [ "$ok" -ne 1 ]; then
    cat /tmp/ch07-cluster.log >&2
    exit 1
fi
echo '==> 创建集群 shorty（kind-1n.yaml）'
for _ in 1 2 3; do
    pull_images busybox:1.37 && break
    sleep 2
done
kubectl apply -f k8s-qos.yaml
kubectl wait --for=condition=Ready pod -l app=ch07-qos --timeout=120s

kubectl get pod -l app=ch07-qos \
    -o custom-columns=NAME:.metadata.name,QOS:.status.qosClass,PHASE:.status.phase

for pod in qos-guaranteed qos-burstable qos-besteffort; do
    printf '%-18s %s\n' "$pod" "$(kubectl get pod "$pod" -o jsonpath='{.status.qosClass}')"
done

kubectl get pod qos-guaranteed \
    -o jsonpath='{.spec.containers[0].resources.requests}{" = "}{.spec.containers[0].resources.limits}{"\n"}'
kubectl get pod qos-burstable \
    -o jsonpath='{.spec.containers[0].resources.requests}{" < "}{.spec.containers[0].resources.limits}{"\n"}'

#> hide
{
    kind delete cluster --name shorty > /dev/null 2>&1 || true
    docker start shorty-pg shorty-redis shorty-api > /dev/null 2>&1 || true
    for _ in $(seq 1 60); do
        curl -fsS localhost:8080/readyz > /dev/null 2>&1 && break
        sleep 1
    done
}
