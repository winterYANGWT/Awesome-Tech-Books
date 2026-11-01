# 16.1 期望状态是 2 个副本；删掉一个 Pod，看控制器是否把它补回来。
kubectl apply -f api-2-replicas.yaml
kubectl -n shorty rollout status deployment/api --timeout=120s
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide

old_pod=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')
old_uid=$(kubectl -n shorty get pod "$old_pod" -o jsonpath='{.metadata.uid}')
echo "被删掉的 Pod：$old_pod"

kubectl -n shorty delete pod "$old_pod" --wait=true

# 等 ReplicaSet 真正补出一个新 UID 的 Pod；这段只是等状态，不显示。
#> hide
for _ in $(seq 1 90); do
  count=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api --no-headers 2>/dev/null | wc -l)
  fresh=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api \
    -o jsonpath='{range .items[*]}{.metadata.uid}{"\n"}{end}' 2>/dev/null \
    | grep -vc "^${old_uid}$" || true)
  if [ "$count" -eq 2 ] && [ "$fresh" -ge 1 ]; then
    break
  fi
  sleep 1
done

# 旧 Pod 已经不存在；新 Pod 的名字和 UID 都不是刚才那个。
kubectl -n shorty get pods -l app.kubernetes.io/name=api \
  -o custom-columns=NAME:.metadata.name,UID:.metadata.uid,STATUS:.status.phase
kubectl -n shorty get pods -l app.kubernetes.io/name=api --no-headers | wc -l

# 旧 Pod 的 GET 返回 NotFound，而 Deployment 的期望副本数仍是 2。
#> fails
#> expect: NotFound
kubectl -n shorty get pod "$old_pod"

kubectl -n shorty get deployment api \
  -o jsonpath='desired={.spec.replicas} ready={.status.readyReplicas} available={.status.availableReplicas}{"\n"}'
