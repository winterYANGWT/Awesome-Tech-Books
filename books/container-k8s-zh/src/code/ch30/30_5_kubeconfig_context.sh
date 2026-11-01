# 把当前 context 的默认 namespace 固定为 shorty，后面的 config view 会看到它。
kubectl config set-context --current --namespace=shorty

# 只读身份：ServiceAccount + 内置 view ClusterRole，只绑在 shorty 里。
kubectl -n shorty create serviceaccount shorty-reader
kubectl -n shorty create rolebinding shorty-reader-view \
  --clusterrole=view --serviceaccount=shorty:shorty-reader

# kubeconfig 的 user 用 token 认证；生产里人用的 OIDC/证书在同一个位置。
TOKEN=$(kubectl -n shorty create token shorty-reader --duration=2h)
kubectl config set-credentials shorty-reader --token="$TOKEN"

# context 只是 cluster + user + namespace 的组合，名字随便起。
kubectl config set-context shorty-reader \
  --cluster=kind-shorty --user=shorty-reader --namespace=shorty

kubectl config get-clusters
kubectl config get-users
kubectl config get-contexts

# 切换 context：同一个 server，换的是 user 和默认 namespace。
kubectl config use-context shorty-reader
kubectl auth whoami
kubectl get pods

# view 允许读，不允许删。
#> fails
#> expect: Forbidden
kubectl delete pod -l app.kubernetes.io/name=api

# 切回 admin context，后面的例子还要建 RBAC 对象。
kubectl config use-context kind-shorty
kubectl auth whoami
