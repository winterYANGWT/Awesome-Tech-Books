kubectl taint node shorty-worker dedicated=shorty:NoSchedule
kubectl apply -f taint-pod.yaml
kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}'=Unschedulable pod/taint-demo --timeout=60s
kubectl -n shorty describe pod taint-demo | sed -n '/Events:/,$p' | head -8
# tolerations 可以在 Pod 创建后添加；调度器下一次重试就会考虑它。
kubectl -n shorty patch pod taint-demo --type merge -p '{"spec":{"tolerations":[{"key":"dedicated","operator":"Equal","value":"shorty","effect":"NoSchedule"}]}}'
kubectl -n shorty wait --for=condition=Ready pod/taint-demo --timeout=120s
kubectl -n shorty get pod taint-demo -o wide
#> hide
kubectl -n shorty delete pod taint-demo --wait=true
#> hide
kubectl taint node shorty-worker dedicated=shorty:NoSchedule-
