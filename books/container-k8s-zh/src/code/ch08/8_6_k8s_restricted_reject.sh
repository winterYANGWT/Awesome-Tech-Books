# 8.6 在 PSA restricted 命名空间里创建 root + privileged 的 Deployment。
# 用一个单节点 kind 集群验证准入和 ReplicaSet 控制器，示例结束就删掉。
source ../lib/common.sh

#> timeout: 600
cluster_up kind-1n.yaml

kubectl create namespace ch08-restricted
kubectl label namespace ch08-restricted \
    pod-security.kubernetes.io/enforce=restricted \
    pod-security.kubernetes.io/warn=restricted \
    pod-security.kubernetes.io/audit=restricted
kubectl get namespace ch08-restricted --show-labels

# Deployment 本身能创建；Pod 由 ReplicaSet 控制器去创建，会被准入拒绝。
kubectl apply -f restricted-violation.yaml

# 等 ReplicaSet 记下 ReplicaFailure，再读 Event。
for _ in $(seq 1 60); do
    msg=$(kubectl -n ch08-restricted get rs \
        -o jsonpath='{range .items[*]}{.status.conditions[?(@.type=="ReplicaFailure")].message}{"\n"}{end}')
    [ -n "$msg" ] && break
    sleep 1
done
kubectl -n ch08-restricted get rs

# Event 里的拒绝原因来自 PodSecurity 准入。
kubectl -n ch08-restricted get events --field-selector reason=FailedCreate \
    -o jsonpath='{.items[0].message}{"\n"}' | cut -c1-600

#> hide
kind delete cluster --name shorty
