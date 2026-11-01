# 26.4 CSI 相关的 API 对象，以及 kind 的 local-path 为什么不是 CSI。
kubectl api-resources --api-group=storage.k8s.io

# 没有 CSI 驱动注册时，这三类对象都是空的。
kubectl get csidriver
kubectl get csinode
kubectl get volumeattachment

# kind 默认 StorageClass 的 provisioner 是 local-path，不是 CSI 驱动名。
kubectl get sc standard -o jsonpath='provisioner={.provisioner}{"\n"}'

# 动态制备的 PV 会记录 provisioned-by 注解。
kubectl get pv -o jsonpath='{range .items[*]}{.metadata.name}{" provisioned-by="}{.metadata.annotations.pv\.kubernetes\.io/provisioned-by}{"\n"}{end}'

# 这个动态 PV 的底层是 hostPath，不是 CSI volume source。
auto_pv=$(kubectl -n dynamic-demo get pvc auto-pvc -o jsonpath='{.spec.volumeName}')
kubectl get pv "$auto_pv" -o jsonpath='pv={.metadata.name} hostPath={.spec.hostPath.path} csi={.spec.csi.driver}{"\n"}'
