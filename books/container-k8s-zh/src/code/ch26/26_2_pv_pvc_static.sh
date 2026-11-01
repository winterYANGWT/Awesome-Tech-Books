# 26.2 手写一个 PV，再用 PVC 申领它。
kubectl apply -f manual-pv-pvc.yaml

# PVC 进入 Bound 才算绑定完成。
kubectl -n storage-demo wait --for=jsonpath='{.status.phase}'=Bound pvc/manual-pvc --timeout=60s

# PV 是集群资源，PVC 属于命名空间；两个列表里的 VOLUME 名字要对得上。
kubectl get pv,pvc -n storage-demo

# PV 预留给了指定的 PVC，PVC 请求 800Mi，实际拿到 1Gi。
kubectl get pv manual-pv -o jsonpath='phase={.status.phase} claim={.spec.claimRef.namespace}/{.spec.claimRef.name} reclaim={.spec.persistentVolumeReclaimPolicy} mode={.spec.volumeMode}{"\n"}'
kubectl -n storage-demo get pvc manual-pvc -o jsonpath='phase={.status.phase} request={.spec.resources.requests.storage} capacity={.status.capacity.storage} class={.spec.storageClassName}{"\n"}'
