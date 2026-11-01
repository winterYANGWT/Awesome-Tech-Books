# 26.3 默认 StorageClass 与延迟绑定。
kubectl get sc
kubectl get sc standard -o custom-columns=NAME:.metadata.name,PROVISIONER:.provisioner,RECLAIM:.reclaimPolicy,BINDING:.volumeBindingMode,EXPANSION:.allowVolumeExpansion
kubectl get sc standard -o jsonpath='{.metadata.annotations}{"\n"}'

# PVC 不写 storageClassName，由默认 StorageClass 补上。
kubectl apply -f dynamic-pvc.yaml
kubectl -n dynamic-demo get pvc auto-pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName,CLASS:.spec.storageClassName

# 默认类使用 WaitForFirstConsumer：没有 Pod 时 PVC 停在 Pending，还没有 PV。
kubectl get pv

# Pod 被调度后，local-path 才在当前节点上制备卷并绑定。
kubectl apply -f dynamic-pod.yaml
kubectl -n dynamic-demo wait --for=condition=Ready pod/auto-pod --timeout=120s

kubectl -n dynamic-demo get pvc auto-pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName,CAPACITY:.status.capacity.storage,CLASS:.spec.storageClassName
kubectl get sc,pv,pvc
