# 35.6 给没有 shell 的 api 加临时容器；再用 kubectl debug node + crictl 看节点。

POD=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.name}')

# api 是 scratch 镜像，exec 进不去：没有 sh，也没有 curl。
#> fails
#> expect: executable file not found
kubectl -n shorty exec $POD -- sh -c 'echo hello'

# 记下加临时容器之前原容器的重启次数。
BEFORE=$(kubectl -n shorty get pod $POD \
  -o jsonpath='{.status.containerStatuses[0].restartCount}')

# 临时容器和 api 共享网络命名空间，所以 localhost:8080 就是 api 自己。
# shorty 开了 PSA restricted，临时容器要用 restricted profile 才进得去。
kubectl -n shorty debug pod/$POD --image=curlimages/curl:8.14.1 \
  --profile=restricted --attach=false -- curl -fsS localhost:8080/healthz
DC=$(kubectl -n shorty get pod $POD -o jsonpath='{.spec.ephemeralContainers[0].name}')

# --attach=false 不会等命令执行完，轮询它的退出码。
for i in $(seq 1 60); do
  EC=$(kubectl -n shorty get pod $POD \
    -o jsonpath='{.status.ephemeralContainerStatuses[0].state.terminated.exitCode}')
  [ -n "$EC" ] && break
  sleep 0.5
done
echo "ephemeral $DC exitCode=$EC"
#> expect: ^ok$
kubectl -n shorty logs $POD -c $DC

# 原容器没有因为这次操作重启：前后 restartCount 相同。
AFTER=$(kubectl -n shorty get pod $POD \
  -o jsonpath='{.status.containerStatuses[0].restartCount}')
echo "api restartCount before=$BEFORE after=$AFTER, ephemeral=$DC"
[ "$BEFORE" = "$AFTER" ]

# 节点上的排障：起一个带 hostPID/网络的特权 Pod，把节点根文件系统挂在 /host。
NODE=$(kubectl -n shorty get pod $POD -o jsonpath='{.spec.nodeName}')
kubectl debug node/$NODE --image=alpine:3.22 --profile=sysadmin --attach=false -- \
  sh -c 'chroot /host crictl ps | grep -E "CONTAINER|shorty$"'
DPOD=$(kubectl get pods --sort-by=.metadata.creationTimestamp -o name | tail -n 1 | cut -d/ -f2)
for i in $(seq 1 60); do
  PHASE=$(kubectl get pod $DPOD -o jsonpath='{.status.phase}')
  [ "$PHASE" = Succeeded ] && break
  sleep 1
done
echo "node debug pod $DPOD phase=$PHASE"
#> head: 9
kubectl logs $DPOD
