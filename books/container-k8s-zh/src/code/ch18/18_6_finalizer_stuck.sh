# 18.6 ownerReference 链和一个卡在 Terminating 的 ConfigMap
#> expect: -> ReplicaSet/
kubectl -n shorty get pods -o json | jq -r '.items[] | "\(.metadata.name) -> \(.metadata.ownerReferences[0].kind)/\(.metadata.ownerReferences[0].name)"'

#> expect: -> Deployment/api
kubectl -n shorty get rs -o json | jq -r '.items[] | "\(.metadata.name) -> \(.metadata.ownerReferences[0].kind)/\(.metadata.ownerReferences[0].name)"'

#> expect: created
kubectl -n shorty create configmap finalizer-demo --from-literal=key=value

#> expect: patched
kubectl -n shorty patch configmap finalizer-demo --type=merge -p '{"metadata":{"finalizers":["example.com/hold"]}}'

# 删除请求被接受，但对象不会消失；kubectl 一直等到超时。
#> fails
#> expect: timed out
kubectl -n shorty delete configmap finalizer-demo --timeout=5s

#> expect: example\.com/hold
kubectl -n shorty get configmap finalizer-demo -o jsonpath='{.metadata.deletionTimestamp}{" "}{.metadata.finalizers}{"\n"}'

#> expect: patched
kubectl -n shorty patch configmap finalizer-demo --type=merge -p '{"metadata":{"finalizers":[]}}'

#> fails
#> expect: NotFound
kubectl -n shorty get configmap finalizer-demo
