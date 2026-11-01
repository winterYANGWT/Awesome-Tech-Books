# 22.1 一个 1 副本的 StatefulSet：名字、DNS、卷各是什么。
kubectl apply -f sts-demo.yaml

# 等控制器把 postgres-0 拉起来。
kubectl -n sts-demo rollout status statefulset/postgres --timeout=120s

# STATE 里能看到 serviceName 和默认的 podManagementPolicy。
kubectl -n sts-demo get statefulset postgres -o custom-columns=NAME:.metadata.name,REPLICAS:.spec.replicas,READY:.status.readyReplicas,SERVICE:.spec.serviceName,POLICY:.spec.podManagementPolicy

# Pod 名是 <StatefulSet 名>-<序号>；PVC 名是 <volumeClaimTemplates 名>-<Pod 名>。
kubectl -n sts-demo get pod,pvc

# Pod 里的 hostname 就是 Pod 名。
kubectl -n sts-demo exec postgres-0 -- hostname

# hostname -f 给出 headless Service 管辖的完整域名。
kubectl -n sts-demo exec postgres-0 -- hostname -f

# 直接查 Pod 的 FQDN：<pod>.<service>.<namespace>.svc.cluster.local。
kubectl -n sts-demo exec postgres-0 -- nslookup postgres-0.postgres.sts-demo.svc.cluster.local

# 查 Service 名本身：headless Service 返回后端 Pod 的地址。
kubectl -n sts-demo exec postgres-0 -- nslookup postgres.sts-demo.svc.cluster.local

# 往卷里写一个只有第一次启动才会写的文件。
kubectl -n sts-demo exec postgres-0 -- sh -c 'cat /data/written-by'

# 记下重建前的 UID、IP，然后删掉 Pod。
old_uid=$(kubectl -n sts-demo get pod postgres-0 -o jsonpath='{.metadata.uid}')
old_ip=$(kubectl -n sts-demo get pod postgres-0 -o jsonpath='{.status.podIP}')
printf 'before: uid=%s ip=%s\n' "$old_uid" "$old_ip"
kubectl -n sts-demo delete pod postgres-0 --wait=true

# 控制器用同一个名字重建，等到 Ready。
kubectl -n sts-demo rollout status statefulset/postgres --timeout=120s

# UID 变了，名字和卷没变。
new_uid=$(kubectl -n sts-demo get pod postgres-0 -o jsonpath='{.metadata.uid}')
new_ip=$(kubectl -n sts-demo get pod postgres-0 -o jsonpath='{.status.podIP}')
printf 'after:  uid=%s ip=%s name=%s\n' "$new_uid" "$new_ip" "$(kubectl -n sts-demo get pod postgres-0 -o jsonpath='{.metadata.name}')"
kubectl -n sts-demo exec postgres-0 -- sh -c 'cat /data/written-by'
