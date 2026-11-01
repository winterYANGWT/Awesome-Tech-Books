# 先把 api 的 requests/limits、PriorityClass 和拓扑分布落到 Deployment 模板上。
kubectl -n shorty patch deployment api --type strategic -p '{"spec":{"template":{"spec":{"priorityClassName":"shorty-api","topologySpreadConstraints":[{"maxSkew":1,"topologyKey":"kubernetes.io/hostname","whenUnsatisfiable":"DoNotSchedule","nodeTaintsPolicy":"Honor","labelSelector":{"matchLabels":{"app.kubernetes.io/name":"api"}}}],"containers":[{"name":"api","resources":{"requests":{"cpu":"250m","memory":"64Mi"},"limits":{"cpu":"500m","memory":"128Mi"}}}]}}}}'
kubectl -n shorty rollout status deployment/api --timeout=180s
old=$(kubectl -n shorty get rs -l app.kubernetes.io/name=api -o jsonpath='{range .items[?(@.spec.replicas==0)]}{.metadata.labels.pod-template-hash}{"\n"}{end}' | head -1)
[ -z "$old" ] || kubectl -n shorty wait --for=delete pod -l pod-template-hash="$old" --timeout=60s
kubectl -n shorty get pod -l app.kubernetes.io/name=api -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,QOS:.status.qosClass,CPU-REQ:.spec.containers[0].resources.requests.cpu,CPU-LIMIT:.spec.containers[0].resources.limits.cpu,MEM-LIMIT:.spec.containers[0].resources.limits.memory
pod=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{.items[0].metadata.name}')
kubectl -n shorty get pod "$pod" -o jsonpath='{.status.qosClass}{" restarts="}{.status.containerStatuses[0].restartCount}{" cpu-limit="}{.status.containerStatuses[0].resources.limits.cpu}{"\n"}'
kubectl -n shorty patch pod "$pod" --subresource resize --type merge -p '{"spec":{"containers":[{"name":"api","resources":{"requests":{"cpu":"250m","memory":"64Mi"},"limits":{"cpu":"1","memory":"128Mi"}}}]}}'
kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].resources.limits.cpu}'=1 pod/"$pod" --timeout=60s
kubectl -n shorty get pod "$pod" -o jsonpath='{.status.qosClass}{" restarts="}{.status.containerStatuses[0].restartCount}{" cpu-limit="}{.status.containerStatuses[0].resources.limits.cpu}{"\n"}'
kubectl -n shorty describe pod "$pod" | grep -E -A2 'Limits:|Requests:|Restart Count:'
#> hide
kubectl -n shorty patch deployment api --type strategic -p '{"spec":{"template":{"spec":{"containers":[{"name":"api","resources":{"requests":{"cpu":"250m","memory":"64Mi"},"limits":{"cpu":"1","memory":"128Mi"}}}]}}}}'
#> hide
kubectl -n shorty rollout status deployment/api --timeout=180s
#> hide
old=$(kubectl -n shorty get rs -l app.kubernetes.io/name=api -o jsonpath='{range .items[?(@.spec.replicas==0)]}{.metadata.labels.pod-template-hash}{"\n"}{end}' | head -1)
#> hide
[ -z "$old" ] || kubectl -n shorty wait --for=delete pod -l pod-template-hash="$old" --timeout=60s
