# 装 Gateway API 1.6.3 的 standard 通道 CRD（只含 GA 的 API kind）。
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.6.3/standard-install.yaml
#> tail: 12

# 名字里带 gateway 的 CRD 都来自这个安装包。
kubectl get crd | grep gateway

# CRD 上的 bundle-version 注解说明装的是哪个版本。
kubectl get crd gateways.gateway.networking.k8s.io \
  -o jsonpath='{.metadata.annotations.gateway\.networking\.k8s\.io/bundle-version}{"\n"}'

# 这些 CRD 注册成了哪几个 API 资源。
kubectl api-resources --api-group=gateway.networking.k8s.io
