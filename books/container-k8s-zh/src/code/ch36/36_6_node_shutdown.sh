# 36.6 节点关机。选一个没有 postgres 的 worker：postgres 的 local-path 卷绑在节点上，
# 节点停掉后它不能漂到另一台。先看 kubelet 的关机和 swap 配置。
PG_NODE=$(kubectl -n shorty get pod -l app.kubernetes.io/name=postgres \
  -o jsonpath='{.items[0].spec.nodeName}')
if [ "$PG_NODE" = shorty-worker ]; then TARGET=shorty-worker2; else TARGET=shorty-worker; fi
echo "postgres 在 $PG_NODE，停 $TARGET"
docker exec "$TARGET" sh -c \
  'grep -E "shutdownGracePeriod|failSwapOn|memorySwap" /var/lib/kubelet/config.yaml'

# 36.4 结束时 maintenance-demo 已删除。这里重新建 4 个 Pod，软反亲和会把它们
# 放在没有 postgres 的那台 worker 上，正好是这次要停的节点。
kubectl apply -f maintenance-demo.yaml
kubectl -n shorty rollout status deployment/maintenance-demo --timeout=120s
kubectl -n shorty get pods -o wide | grep -E 'NAME|maintenance-demo'

#> timeout: 240
# 停掉节点容器，模拟机器直接断电。kubelet 不再续 Lease，节点进入 NotReady。
docker stop "$TARGET"
for i in $(seq 1 60); do
  STATE=$(kubectl get node "$TARGET" \
    -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}')
  if [ "$STATE" != True ]; then break; fi
  sleep 2
done
echo "Ready condition: $STATE"
kubectl get node "$TARGET"
# 节点失联后，Pod 对象还在，kubelet 已经不在，没人删它们。
kubectl -n shorty get pods -o wide --field-selector "spec.nodeName=$TARGET"

#> timeout: 240
# 非优雅关机：给节点打 out-of-service 污点，强制删除不能容忍它的 Pod，
# 并立即做卷 detach。NodeOutOfServiceVolumeDetach 在 1.28 稳定。
kubectl taint node "$TARGET" node.kubernetes.io/out-of-service=nodeshutdown:NoExecute
kubectl -n shorty rollout status deployment/api --timeout=120s
kubectl -n shorty rollout status deployment/maintenance-demo --timeout=120s
for i in $(seq 1 60); do
  LEFT=$(kubectl -n shorty get pods --field-selector "spec.nodeName=$TARGET" \
    --no-headers 2>/dev/null | wc -l)
  if [ "$LEFT" -eq 0 ]; then break; fi
  sleep 2
done
echo "原节点上剩余 Pod: $LEFT"
kubectl -n shorty get pods -o wide

#> timeout: 300
# 节点恢复后先等 Ready，再摘掉 out-of-service 污点，让调度恢复。
docker start "$TARGET"
kubectl wait --for=condition=Ready node/"$TARGET" --timeout=180s
kubectl taint node "$TARGET" node.kubernetes.io/out-of-service-
kubectl get nodes
kubectl -n shorty delete deployment maintenance-demo
