# 31.5 hostUsers: false 的 Pod 能通过 PSA：user namespace 里的 uid 0 不是节点上的 root。
kubectl apply -f userns-pod.yaml
kubectl -n shorty get pod userns-demo -o jsonpath='hostUsers={.spec.hostUsers} runAsUser={.spec.containers[0].securityContext.runAsUser}{"\n"}'

# 能启动就进容器看 id；起不来就读 kubelet 写的事件。
# lab 是嵌套容器，节点内核不一定会允许这里的 idmap mount，两种结果都算正常。
if kubectl -n shorty wait --for=condition=Ready pod/userns-demo --timeout=30s >/dev/null 2>&1; then
  kubectl -n shorty exec userns-demo -- id
else
  for i in $(seq 1 30); do
    msg=$(kubectl -n shorty get events --field-selector involvedObject.name=userns-demo \
      -o jsonpath='{range .items[*]}{.reason}{": "}{.message}{"\n"}{end}' \
      | grep -E 'FailedCreatePodSandBox|FailedCreate' || true)
    [ -n "$msg" ] && break
    sleep 1
  done
  printf '%s\n' "$msg" | head -1
fi

# 删掉这个实验 Pod；它如果在失败重试，会一直产生同样的事件。
kubectl -n shorty delete pod userns-demo --ignore-not-found
