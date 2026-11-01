# 当前 kubeconfig 里的证书被 apiserver 认成了谁。
kubectl auth whoami

# 只看当前 context：它把 cluster、user 和 namespace 绑在一起。
kubectl config view --minify

# 这个组不是 system:masters，而是 kubeadm 建集群时显式绑到 cluster-admin 的组。
kubectl get clusterrolebinding kubeadm:cluster-admins \
  -o jsonpath='{.roleRef.name}{"\n"}{.subjects[0].kind}/{.subjects[0].name}{"\n"}'

# 所以当前身份对任何资源、任何动词都是 yes。
kubectl auth can-i '*' '*' --all-namespaces

# 匿名请求没有凭据，RBAC 不会给它读 Pod 的权限。
kubectl create -f sar-anonymous.yaml \
  -o jsonpath='{.status.allowed}{"\n"}'
