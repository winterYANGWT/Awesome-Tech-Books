# 26.6 默认 StorageClass 没开 allowVolumeExpansion，扩容请求被拒绝。
kubectl -n dynamic-demo get pvc auto-pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,REQUEST:.spec.resources.requests.storage,CAPACITY:.status.capacity.storage

kubectl get sc standard -o custom-columns=NAME:.metadata.name,PROVISIONER:.provisioner,ALLOWVOLUMEEXPANSION:.allowVolumeExpansion

#> fails
#> expect: only dynamically provisioned pvc can be resized and the storageclass that provisions the pvc must support resize
kubectl -n dynamic-demo patch pvc auto-pvc --type=merge -p '{"spec":{"resources":{"requests":{"storage":"2Gi"}}}}'

# 失败后 PVC 仍是 1Gi。
kubectl -n dynamic-demo get pvc auto-pvc -o jsonpath='request={.spec.resources.requests.storage} capacity={.status.capacity.storage}{"\n"}'
