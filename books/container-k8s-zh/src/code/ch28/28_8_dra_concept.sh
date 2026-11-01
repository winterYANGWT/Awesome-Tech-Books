kubectl api-resources --api-group=resource.k8s.io
# DRA 的 API 在，但 lab 里没有 DRA 驱动，所以没有任何 DeviceClass 和 ResourceSlice。
kubectl get deviceclass,resourceslice
kubectl apply -f dra-claim.yaml
kubectl apply -f dra-pod.yaml
kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}'=Unschedulable pod/dra-demo --timeout=60s
kubectl -n shorty describe pod dra-demo | sed -n '/Events:/,$p' | head -8
kubectl -n shorty get resourceclaim gpu-claim -o jsonpath='{.status.allocation}{"\n"}'
#> hide
kubectl -n shorty delete pod/dra-demo resourceclaim/gpu-claim --ignore-not-found --wait=true
