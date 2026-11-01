# 26.7 三种 Pending：类不存在、显式空类、等待 Pod 调度。
kubectl apply -f pending-pvcs.yaml
kubectl -n trouble-demo get pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName,CLASS:.spec.storageClassName

# 等控制器写出事件，再读 describe 的 Events 段。
#> hide
for i in $(seq 1 30); do
  a=$(kubectl -n trouble-demo get events --field-selector involvedObject.kind=PersistentVolumeClaim,involvedObject.name=missing-class --no-headers 2>/dev/null | wc -l)
  b=$(kubectl -n trouble-demo get events --field-selector involvedObject.kind=PersistentVolumeClaim,involvedObject.name=no-class --no-headers 2>/dev/null | wc -l)
  c=$(kubectl -n trouble-demo get events --field-selector involvedObject.kind=PersistentVolumeClaim,involvedObject.name=waiting --no-headers 2>/dev/null | wc -l)
  [ "$a" -gt 0 ] && [ "$b" -gt 0 ] && [ "$c" -gt 0 ] && break
  sleep 1
done

kubectl -n trouble-demo describe pvc missing-class | sed -n '/^Events:/,$p'
kubectl -n trouble-demo describe pvc no-class | sed -n '/^Events:/,$p'
kubectl -n trouble-demo describe pvc waiting | sed -n '/^Events:/,$p'
