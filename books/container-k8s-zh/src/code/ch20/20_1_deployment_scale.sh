kubectl -n shorty get deployment api
# 扩容不触发新的 ReplicaSet：Deployment 只改现有 RS 的 replicas。
kubectl -n shorty scale deployment api --replicas=4
#> tail: 2
kubectl -n shorty rollout status deployment/api --timeout=120s
kubectl -n shorty get rs
kubectl -n shorty get pods -o wide
# 所有权链：RS 归 Deployment，Pod 归 RS。
kubectl -n shorty get rs -o jsonpath='{range .items[*]}{.metadata.name}{" owner="}{.metadata.ownerReferences[0].kind}{"/"}{.metadata.ownerReferences[0].name}{"\n"}{end}'
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{range .items[*]}{.metadata.name}{" owner="}{.metadata.ownerReferences[0].kind}{"/"}{.metadata.ownerReferences[0].name}{"\n"}{end}'
# 直接改 RS 的 replicas，Deployment 会把它改回 4。
RS=$(kubectl -n shorty get rs -l app.kubernetes.io/name=api -o jsonpath='{range .items[?(@.spec.replicas>0)]}{.metadata.name}{"\n"}{end}')
echo "RS=$RS"
kubectl -n shorty scale rs "$RS" --replicas=1
kubectl -n shorty wait --for=jsonpath='{.spec.replicas}'=4 "rs/$RS" --timeout=30s
kubectl -n shorty wait --for=jsonpath='{.status.readyReplicas}'=4 "rs/$RS" --timeout=60s
kubectl -n shorty get rs
