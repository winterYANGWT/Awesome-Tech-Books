# 26.6 reclaimPolicy: Delete 的动态卷：PVC 删除后 PV 也删除。
kubectl apply -f reclaim-delete-sc.yaml
kubectl apply -f reclaim-delete.yaml
kubectl -n reclaim-demo wait --for=condition=Ready pod/delete-pod --timeout=120s

delete_pv=$(kubectl -n reclaim-demo get pvc delete-pvc -o jsonpath='{.spec.volumeName}')
printf 'pvc=delete-pvc pv=%s reclaim=%s\n' "$delete_pv" \
  "$(kubectl get pv "$delete_pv" -o jsonpath='{.spec.persistentVolumeReclaimPolicy}')"

kubectl -n reclaim-demo delete pod delete-pod --wait=true --timeout=90s
kubectl -n reclaim-demo delete pvc delete-pvc --wait=true --timeout=90s

# PV 由控制器删除；等它真的消失，而不是先假设。
kubectl wait --for=delete "pv/$delete_pv" --timeout=60s

#> fails
#> expect: NotFound
kubectl get pv "$delete_pv"
