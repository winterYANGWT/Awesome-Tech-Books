kubectl apply -f gate-pod.yaml
kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}'=SchedulingGated pod/gate-demo --timeout=60s
kubectl -n shorty get pod gate-demo
kubectl -n shorty get pod gate-demo -o jsonpath='{.spec.schedulingGates}{"\n"}'
kubectl -n shorty get pod gate-demo -o jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}{" "}{.status.conditions[?(@.type=="PodScheduled")].message}{"\n"}'
# 删除最后一个 gate，调度器才会看到这个 Pod。
kubectl -n shorty patch pod gate-demo --type=json -p='[{"op":"remove","path":"/spec/schedulingGates"}]'
kubectl -n shorty wait --for=condition=Ready pod/gate-demo --timeout=120s
kubectl -n shorty get pod gate-demo -o wide
kubectl -n shorty get events --field-selector involvedObject.name=gate-demo --sort-by=.lastTimestamp -o custom-columns=REASON:.reason,MESSAGE:.message
