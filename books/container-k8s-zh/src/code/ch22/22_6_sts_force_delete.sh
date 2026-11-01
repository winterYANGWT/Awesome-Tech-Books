# 22.6 强制删除：名字立刻释放，控制器马上用同一个名字建新 Pod。
old_uid=$(kubectl -n shorty get pod postgres-0 -o jsonpath='{.metadata.uid}')
old_ip=$(kubectl -n shorty get pod postgres-0 -o jsonpath='{.status.podIP}')
printf 'before: uid=%s ip=%s\n' "$old_uid" "$old_ip"

# --force --grace-period=0 不等待 kubelet 确认容器已经停止。
kubectl -n shorty delete pod postgres-0 --force --grace-period=0

# 轮询到 UID 变化，确认拿到的是新 Pod，不是还没消失的旧对象。
for i in $(seq 1 120); do
  uid=$(kubectl -n shorty get pod postgres-0 -o jsonpath='{.metadata.uid}' 2>/dev/null || true)
  [ -n "$uid" ] && [ "$uid" != "$old_uid" ] && break
  sleep 1
done
kubectl -n shorty wait --for=condition=Ready pod/postgres-0 --timeout=180s

new_uid=$(kubectl -n shorty get pod postgres-0 -o jsonpath='{.metadata.uid}')
new_ip=$(kubectl -n shorty get pod postgres-0 -o jsonpath='{.status.podIP}')
printf 'after:  uid=%s ip=%s name=postgres-0\n' "$new_uid" "$new_ip"

# 名字相同、PVC 相同，说明 StatefulSet 把身份重新挂到了新 Pod 上。
kubectl -n shorty get pod postgres-0 -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,IP:.status.podIP
kubectl -n shorty get pvc data-postgres-0

# 风险演示：第二个 postmaster 挂同一个数据目录，Kubernetes 不会拦它。
kubectl apply -f pg-duplicate.yaml
for i in $(seq 1 120); do
  if kubectl -n shorty logs pg-duplicate 2>/dev/null | grep -q 'database system is ready to accept connections'; then
    break
  fi
  phase=$(kubectl -n shorty get pod pg-duplicate -o jsonpath='{.status.phase}' 2>/dev/null || true)
  [ "$phase" = Failed ] && break
  sleep 1
done
kubectl -n shorty get pod pg-duplicate -o wide
#> expect: database system is ready to accept connections
kubectl -n shorty logs pg-duplicate --tail=40 || true
kubectl -n shorty delete pod pg-duplicate --wait=true
