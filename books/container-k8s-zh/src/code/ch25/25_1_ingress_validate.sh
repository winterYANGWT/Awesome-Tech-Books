# 起点确认：集群里没有 IngressClass，也没有 Ingress 对象。
kubectl get ingressclass
kubectl -n shorty get ingress

# 只在服务端验证 schema，不落盘；此时并没有控制器去实现这份规则。
kubectl apply --dry-run=server -f ingress.yaml

# dry-run 之后对象仍然不存在，说明 Ingress 只是声明，不是数据面。
kubectl -n shorty get ingress

# pathType 是必填字段；去掉它，同一个对象被 apiserver 拒绝。
#> fails
#> expect: spec.rules\[0\].http.paths\[0\].pathType: Required value
kubectl apply --dry-run=server -f ingress-no-pathtype.yaml

# 看看这个字段在 API 里的取值。
kubectl explain ingress.spec.rules.http.paths.pathType
