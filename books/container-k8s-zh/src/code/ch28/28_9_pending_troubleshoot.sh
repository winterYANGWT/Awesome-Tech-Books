kubectl apply -f pending-pods.yaml
for p in too-big control-plane-only missing-pvc; do
  kubectl -n ch28-pending wait --for=jsonpath='{.status.conditions[?(@.type=="PodScheduled")].reason}'=Unschedulable pod/"$p" --timeout=60s
  echo "== $p"
  kubectl -n ch28-pending describe pod "$p" | grep -A1 'FailedScheduling' | tail -1
done
#> hide
kubectl delete namespace ch28-pending --wait=true
#> hide
kubectl -n shorty delete pod gate-demo --ignore-not-found --wait=true
#> hide
kubectl delete namespace ch28-qos --wait=true
#> hide
kubectl delete priorityclass shorty-low --ignore-not-found
