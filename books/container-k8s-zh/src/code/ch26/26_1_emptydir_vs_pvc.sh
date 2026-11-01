# 26.1 两个 Pod：一个用 emptyDir，一个用 PVC。
kubectl apply -f vol-demo.yaml

kubectl -n vol-demo wait --for=condition=Ready pod/cache-pod pod/keep-pod --timeout=120s

# 各写一个标记文件。
kubectl -n vol-demo exec cache-pod -- sh -c 'echo cache-first-run > /data/marker'
kubectl -n vol-demo exec keep-pod -- sh -c 'echo pvc-first-run > /data/marker'

# PVC 已经因为 keep-pod 的调度而绑定。
kubectl -n vol-demo get pvc keep-data -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName,CAPACITY:.status.capacity.storage,CLASS:.spec.storageClassName

# 删掉两个 Pod。PVC 不是 Pod 的一部分，会留下来。
kubectl -n vol-demo delete pod cache-pod keep-pod --wait=true

# 用同一份清单重建两个 Pod。
kubectl apply -f vol-demo.yaml
kubectl -n vol-demo wait --for=condition=Ready pod/cache-pod pod/keep-pod --timeout=120s

# emptyDir 随旧 Pod 消失；PVC 里的文件还在。
printf 'cache-pod: '; kubectl -n vol-demo exec cache-pod -- sh -c 'test -f /data/marker && cat /data/marker || echo missing'
printf 'keep-pod:  '; kubectl -n vol-demo exec keep-pod -- sh -c 'test -f /data/marker && cat /data/marker || echo missing'
