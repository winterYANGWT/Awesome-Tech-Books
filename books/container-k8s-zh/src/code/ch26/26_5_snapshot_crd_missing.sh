# 26.5 kind 默认没有 VolumeSnapshot CRD。
kubectl get crd -o name | grep 'snapshot.storage.k8s.io' || echo '没有 snapshot.storage.k8s.io 的 CRD'

# 没有 CRD，apply 会在客户端或 apiserver 发现不了这个 kind。
#> fails
#> expect: no matches for kind "VolumeSnapshot"
kubectl apply -f volume-snapshot.yaml

# 直接 get 也一样：apiserver 没有这个资源类型。
#> fails
#> expect: doesn't have a resource type
kubectl -n shorty get volumesnapshot
