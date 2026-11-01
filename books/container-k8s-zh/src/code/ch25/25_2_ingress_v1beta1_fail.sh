# 先按旧教程的 extensions/v1beta1 提交，apiserver 直接报找不到这个 kind。
#> fails
#> expect: no matches for kind "Ingress" in version "extensions/v1beta1"
kubectl apply -f ingress-v1beta1.yaml

# 同样的 host/path 换成 networking.k8s.io/v1，apiserver 接受。
kubectl apply -f ingress.yaml
kubectl -n shorty get ingress

# ADDRESS 为空、Events 为空：对象存在，但没有控制器监听它。
kubectl -n shorty describe ingress shorty
