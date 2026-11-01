kubectl label node shorty-worker shorty.example.com/pool=api --overwrite
kubectl -n shorty patch deployment api --type merge -p '{"spec":{"template":{"spec":{"affinity":{"nodeAffinity":{"requiredDuringSchedulingIgnoredDuringExecution":{"nodeSelectorTerms":[{"matchExpressions":[{"key":"shorty.example.com/pool","operator":"In","values":["api"]}]}]}}}}}}}'
kubectl -n shorty rollout status deployment/api --timeout=180s
# 等旧 ReplicaSet 的 Pod 退干净，避免输出里混进正在终止的旧副本。
old=$(kubectl -n shorty get rs -l app.kubernetes.io/name=api -o jsonpath='{range .items[?(@.spec.replicas==0)]}{.metadata.labels.pod-template-hash}{"\n"}{end}' | head -1)
[ -z "$old" ] || kubectl -n shorty wait --for=delete pod -l pod-template-hash="$old" --timeout=60s
# 去掉节点 label 后，已经在跑的 Pod 仍然留在原节点：IgnoredDuringExecution。
kubectl label node shorty-worker shorty.example.com/pool-
kubectl -n shorty get pod -l app.kubernetes.io/name=api -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,STATUS:.status.phase
kubectl apply -f affinity-pending.yaml
kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}'=Unschedulable pod/affinity-pending --timeout=60s
kubectl -n shorty describe pod affinity-pending | sed -n '/Events:/,$p' | head -8
#> hide
kubectl -n shorty delete pod affinity-pending --wait=true
#> hide
kubectl label node shorty-worker shorty.example.com/pool=api --overwrite
#> hide
kubectl -n shorty patch deployment api --type merge -p '{"spec":{"template":{"spec":{"affinity":null}}}}'
#> hide
kubectl -n shorty rollout status deployment/api --timeout=180s
#> hide
kubectl label node shorty-worker shorty.example.com/pool-
