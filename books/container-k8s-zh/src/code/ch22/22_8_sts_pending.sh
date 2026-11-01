# 22.7 排障：StorageClass 不存在时，StatefulSet 的 Pod 会停在 Pending。
kubectl apply -f sts-pending.yaml

# 等控制器把 Pod 建出来；它不会 Ready，只会 Pending。
# kubectl wait 对还不存在的对象会立刻报 NotFound，所以先等它被创建。
#> hide
kubectl -n sts-demo wait --for=create pod/postgres-bad-0 --timeout=60s
kubectl -n sts-demo wait --for=jsonpath='{.status.phase}'=Pending pod/postgres-bad-0 --timeout=60s

# Pod 一行，PVC 也是 Pending，说明卡在存储而不是容器。
kubectl -n sts-demo get pod postgres-bad-0
kubectl -n sts-demo get pvc data-postgres-bad-0

# PVC 的事件直接点名不存在的 StorageClass。
kubectl -n sts-demo describe pvc data-postgres-bad-0 | tail -n 8

# Pod 的调度事件说明它在等一个不会绑定的 PVC。
kubectl -n sts-demo describe pod postgres-bad-0 | tail -n 10

# 清理这个演示对象；namespace sts-demo 留到最后统一删。
kubectl -n sts-demo delete statefulset postgres-bad --wait=true
kubectl -n sts-demo delete pvc data-postgres-bad-0 --wait=true
kubectl delete namespace sts-demo --wait=true
