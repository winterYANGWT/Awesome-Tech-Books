#> timeout: 900
# 24.3 按 Calico 的要求重建集群：不装默认 CNI，节点先停在 NotReady。
source ../lib/common.sh
cluster_up kind-2n-nocni.yaml
app_images

# 镜像先放进 lab 的 Docker，重建后的节点再加载，避免安装时现拉。
# 这一步的输出对读者不重要，隐藏。
#> hide
#> timeout: 600
pull_images \
  postgres:18.6-alpine redis:8.8.4-alpine busybox:1.37 \
  quay.io/tigera/operator:v1.44.0 \
  quay.io/calico/node:v3.33.0 \
  quay.io/calico/calico:v3.33.0 \
  quay.io/calico/third-party-cni-plugins:v3.33.0

# 没有 CNI 时节点 NotReady，Pod 也起不来；先把 s3 清单 apply 进去。
kubectl get nodes
kubectl apply -f ../stages/s3-stateful
kubectl -n shorty get pods

# 节点已经拿到 PodCIDR，但没有 CNI 去配置 veth、路由和 IPAM。
kubectl get nodes -o custom-columns=NAME:.metadata.name,PODCIDR:.spec.podCIDR

#> timeout: 900
# 安装 tigera-operator 和最小 Installation；IPPool 与 kind 的 10.244.0.0/16 对齐。
kubectl apply -f tigera-operator.yaml >/dev/null
kubectl -n tigera-operator wait --for=condition=Available deployment/tigera-operator --timeout=600s
# Operator 会创建 Installation CRD；CRD 没出现之前 kubectl wait 会直接 NotFound，所以先轮询。
for i in $(seq 1 60); do
  kubectl get crd installations.operator.tigera.io >/dev/null 2>&1 && break
  sleep 2
done
kubectl wait --for=condition=Established crd/installations.operator.tigera.io --timeout=120s
kubectl apply -f calico-installation.yaml

#> timeout: 900
# 等 calico-node 在每台节点上出现并 Ready；2 节点集群应该有 2 个。
for i in $(seq 1 60); do
  [ "$(kubectl -n calico-system get pods -l k8s-app=calico-node --no-headers 2>/dev/null | wc -l)" -ge 2 ] && break
  sleep 2
done
kubectl -n calico-system wait --for=condition=Ready pod -l k8s-app=calico-node --timeout=600s
kubectl -n calico-system wait --for=condition=Available deployment/calico-kube-controllers --timeout=600s
kubectl -n calico-system get pods -o wide
kubectl get nodes

#> timeout: 900
# 之前 Pending 的 s3 工作负载现在应该起来了。
kubectl -n shorty rollout status deployment/api --timeout=600s
kubectl -n shorty rollout status deployment/redis --timeout=600s
kubectl -n shorty rollout status deployment/worker --timeout=600s
kubectl -n shorty rollout status statefulset/postgres --timeout=600s
kubectl -n shorty wait --for=condition=Complete job/shorty-migrate --timeout=600s
kubectl -n shorty get pods -o wide

# calicoctl 读的是 Calico 自己的 IPPool；这里应该和节点 PodCIDR 同属 10.244.0.0/16。
calicoctl get ippool -o wide
