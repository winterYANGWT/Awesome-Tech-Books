# 一个和 worker 共用网络栈的临时 Pod，用来从“集群里”访问节点上的 kubelet。
kubectl apply -f kubelet-probe.yaml
kubectl wait --for=condition=Ready pod/kubelet-probe --timeout=120s

# 直接打 kubelet 的 /pods，不经过 apiserver；没有凭据应当被拒绝。
#> fails
#> expect: (401|403|Unauthorized|Forbidden)
kubectl exec kubelet-probe -- wget -O- --no-check-certificate https://127.0.0.1:10250/pods

# kubeadm 把匿名访问关掉，并要求通过 apiserver 的 TokenReview/SubjectAccessReview 鉴权。
docker exec shorty-worker sh -c \
  'grep -A3 -E "^(authentication|authorization):" /var/lib/kubelet/config.yaml'

kubectl delete pod kubelet-probe
