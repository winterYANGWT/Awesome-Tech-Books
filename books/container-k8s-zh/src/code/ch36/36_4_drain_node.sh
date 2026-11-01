# 36.4 节点维护。建 4 个只睡觉的 Pod，软反亲和会尽量避开 postgres 节点。
kubectl apply -f maintenance-demo.yaml
kubectl -n shorty rollout status deployment/maintenance-demo --timeout=120s
kubectl -n shorty get pods -o wide | grep -E 'NAME|maintenance-demo'

# postgres 用的是 local-path 卷，卷绑在节点上；换一个 worker 做维护，避免动它。
PG_NODE=$(kubectl -n shorty get pod -l app.kubernetes.io/name=postgres \
  -o jsonpath='{.items[0].spec.nodeName}')
if [ "$PG_NODE" = shorty-worker ]; then TARGET=shorty-worker2; else TARGET=shorty-worker; fi
echo "postgres 在 $PG_NODE，维护 $TARGET"

#> expect: cordoned
#> expect: drained
#> timeout: 240
# cordon 停止新调度，drain 在它之上驱逐 Pod；DaemonSet 和 emptyDir 要显式放行。
kubectl drain "$TARGET" --ignore-daemonsets --delete-emptydir-data --timeout=180s

#> expect: SchedulingDisabled
kubectl get nodes

# 被驱逐的 Pod 被调度到另一个 worker；原节点上的普通 Pod 应该已经空了。
kubectl -n shorty get pods -o wide | grep -E 'NAME|maintenance-demo'
kubectl -n shorty get pods --field-selector "spec.nodeName=$TARGET"

#> expect: uncordoned
kubectl uncordon "$TARGET"
kubectl get nodes

# uncordon 只是允许新 Pod 调度，不会把已经在别处的 Pod 搬回来。
kubectl -n shorty get pods -o wide | grep -E 'NAME|maintenance-demo'
kubectl -n shorty delete deployment maintenance-demo
