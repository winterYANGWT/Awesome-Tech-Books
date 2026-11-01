# 18.8 删除名字空间卡在 Terminating
kubectl create namespace stuck-demo

kubectl -n stuck-demo create configmap stuck-cm --from-literal=key=value

#> expect: patched
kubectl -n stuck-demo patch configmap stuck-cm --type=merge -p '{"metadata":{"finalizers":["example.com/hold"]}}'

#> fails
#> expect: timed out
kubectl delete namespace stuck-demo --timeout=10s

#> expect: Terminating
kubectl get namespace stuck-demo -o jsonpath='{.status.phase}{"\n"}'

# 逐个资源类型列出该名字空间里还带 deletionTimestamp 的对象。
#> expect: configmaps/stuck-cm finalizers=\["example\.com/hold"\]
for r in $(kubectl api-resources --verbs=list --namespaced -o name); do
  kubectl -n stuck-demo get "$r" -o json 2>/dev/null |
    jq -r --arg r "$r" '.items[]? | select(.metadata.deletionTimestamp != null) | "\($r)/\(.metadata.name) finalizers=\(.metadata.finalizers)"'
done

#> expect: patched
kubectl -n stuck-demo patch configmap stuck-cm --type=merge -p '{"metadata":{"finalizers":[]}}'

#> expect: condition met
kubectl wait --for=delete namespace/stuck-demo --timeout=60s

#> fails
#> expect: NotFound
kubectl get namespace stuck-demo
