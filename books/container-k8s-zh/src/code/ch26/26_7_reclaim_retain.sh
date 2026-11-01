# 26.6 同一个 PVC 换成 reclaimPolicy: Retain：PVC 删除后 PV 留在 Released。
kubectl apply -f reclaim-retain-sc.yaml
kubectl apply -f reclaim-retain.yaml
kubectl -n reclaim-demo wait --for=condition=Ready pod/retain-pod --timeout=120s

retain_pv=$(kubectl -n reclaim-demo get pvc retain-pvc -o jsonpath='{.spec.volumeName}')
printf 'pvc=retain-pvc pv=%s reclaim=%s\n' "$retain_pv" \
  "$(kubectl get pv "$retain_pv" -o jsonpath='{.spec.persistentVolumeReclaimPolicy}')"

kubectl -n reclaim-demo delete pod retain-pod --wait=true --timeout=90s
kubectl -n reclaim-demo delete pvc retain-pvc --wait=true --timeout=90s

kubectl wait --for=jsonpath='{.status.phase}'=Released "pv/$retain_pv" --timeout=60s
kubectl get pv "$retain_pv" -o custom-columns=NAME:.metadata.name,PHASE:.status.phase,RECLAIM:.spec.persistentVolumeReclaimPolicy,CLAIM:.spec.claimRef.name
