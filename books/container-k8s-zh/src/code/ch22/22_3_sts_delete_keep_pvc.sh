# 22.3 删除 StatefulSet，PVC 默认保留。
kubectl -n sts-demo delete statefulset postgres --wait=true

# 等 Pod 也删完；StatefulSet 删除会级联删除 Pod。
kubectl -n sts-demo wait --for=delete pod/postgres-0 --timeout=60s

# 两个 PVC 都还在，状态是 Bound。
kubectl -n sts-demo get pvc

# 没有 StatefulSet 了，PVC 仍然占着 PV。
kubectl -n sts-demo get statefulset
