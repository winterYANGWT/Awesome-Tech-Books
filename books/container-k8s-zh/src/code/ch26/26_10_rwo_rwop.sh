# 26.7 RWO 允许同一节点上的第二个 Pod；RWOP 不允许第二个 Pod。
kubectl apply -f access-a.yaml
kubectl -n access-demo wait --for=condition=Ready pod/rwo-a pod/rwop-a --timeout=120s
kubectl -n access-demo get pods -o wide

# 两个新 Pod 和各自的第一个 Pod 落在同一个节点。
kubectl apply -f access-b.yaml
kubectl -n access-demo wait --for=condition=Ready pod/rwo-b --timeout=120s
kubectl -n access-demo get pods -o wide

# 等调度器给 rwop-b 写出拒绝原因。
#> hide
for i in $(seq 1 60); do
  kubectl -n access-demo describe pod rwop-b 2>/dev/null | grep -Eq 'volume is already used|ReadWriteOncePod|did.not find available persistent volumes' && break
  sleep 1
done

kubectl -n access-demo describe pod rwop-b | sed -n '/^Events:/,$p'
