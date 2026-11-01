# 3.5 RuntimeClass 指向一个节点上没有的 handler
# 先不创建 RuntimeClass 对象，Pod 会在准入阶段被拒。
#> fails
#> expect: RuntimeClass "gvisor" not found
kubectl apply -f pod-gvisor.yaml

# 创建 RuntimeClass 对象，handler 写成 runsc；节点上并没有这个 handler。
kubectl apply -f runtimeclass-gvisor.yaml
kubectl get runtimeclass gvisor -o yaml

# 这次 Pod 对象能创建，但 kubelet 建 sandbox 时会失败。
kubectl apply -f pod-gvisor.yaml
kubectl wait --for=jsonpath='{.status.conditions[?(@.type=="PodReadyToStartContainers")].reason}'=PodSandboxNotReady \
  pod/gvisor-demo -n shorty --timeout=60s
kubectl get pod -n shorty gvisor-demo -o wide

# 失败的 sandbox 不会出现在 crictl 的 Pod 列表里；这里还是只有 shorty 一个。
NODE=$(kind get nodes --name shorty)
docker exec "$NODE" crictl pods --namespace shorty
kubectl get pod -n shorty gvisor-demo -o jsonpath='{.status.phase}{"\n"}{.status.conditions[?(@.type=="PodReadyToStartContainers")].message}{"\n"}'

# 事件没有 condition 可以 wait，等它出现在 API 里再查询，避免读到空列表。
#> hide
for i in $(seq 1 20); do
  kubectl get events -n shorty --field-selector involvedObject.name=gvisor-demo \
    | grep -q FailedCreatePodSandBox && break
  sleep 0.5
done

# 事件里能看到失败原因，注意 handler 名是 runsc。
#> expect: no runtime for "runsc" is configured
kubectl get events -n shorty --field-selector involvedObject.name=gvisor-demo --sort-by=.lastTimestamp
