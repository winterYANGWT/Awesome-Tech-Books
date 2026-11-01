# 22.4 显式设置 persistentVolumeClaimRetentionPolicy，让 PVC 跟着 StatefulSet 删除。
kubectl apply -f sts-demo-retention.yaml
kubectl -n sts-demo rollout status statefulset/postgres --timeout=120s

# 两个副本各有一个 PVC，控制器都给它们加上了指向 StatefulSet 的 ownerReference。
kubectl -n sts-demo get pvc -o custom-columns=NAME:.metadata.name,OWNER-KIND:.metadata.ownerReferences[0].kind,OWNER-NAME:.metadata.ownerReferences[0].name

# 删除 StatefulSet，whenDeleted: Delete 会在 Pod 终止后回收 PVC。
kubectl -n sts-demo delete statefulset postgres --wait=true

# 等两个 PVC 都被垃圾回收；它们底下的 PV 也会按 StorageClass 的回收策略处理。
kubectl -n sts-demo wait --for=delete pvc/data-postgres-0 --timeout=120s
kubectl -n sts-demo wait --for=delete pvc/data-postgres-1 --timeout=120s
kubectl -n sts-demo get pvc
