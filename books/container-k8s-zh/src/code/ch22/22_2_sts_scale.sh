# 22.2 缩放：加一个副本，再缩回去。
# 先看当前只有一个 postgres-0，以及它的创建时间。
kubectl -n sts-demo get pod -o custom-columns=NAME:.metadata.name,READY:.status.containerStatuses[0].ready,CREATED:.metadata.creationTimestamp

# 1 -> 2。OrderedReady 下，控制器先确认 postgres-0 Ready，再创建 postgres-1。
kubectl -n sts-demo scale statefulset postgres --replicas=2

# 等两个副本都 Ready。
kubectl -n sts-demo rollout status statefulset/postgres --timeout=120s

# 两个 Pod 的创建时间：postgres-1 在 postgres-0 之后。
kubectl -n sts-demo get pod -o custom-columns=NAME:.metadata.name,READY:.status.containerStatuses[0].ready,CREATED:.metadata.creationTimestamp

# volumeClaimTemplates 为每个 Pod 生成一个 PVC，名字是 data-<pod>。
kubectl -n sts-demo get pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName

# 2 -> 1。控制器按序号从大到小终止，postgres-1 先走。
kubectl -n sts-demo scale statefulset postgres --replicas=1
kubectl -n sts-demo wait --for=delete pod/postgres-1 --timeout=120s
kubectl -n sts-demo wait --for=jsonpath='{.status.readyReplicas}'=1 statefulset/postgres --timeout=60s

# postgres-1 没了，但 data-postgres-1 还在；这是默认的 Retain 行为。
kubectl -n sts-demo get pod,pvc

# 序号、当前副本数和更新策略都能在 StatefulSet 状态里看到。
kubectl -n sts-demo get statefulset postgres -o custom-columns=NAME:.metadata.name,REPLICAS:.spec.replicas,CURRENT:.status.currentReplicas,READY:.status.readyReplicas,STRATEGY:.spec.updateStrategy.type
