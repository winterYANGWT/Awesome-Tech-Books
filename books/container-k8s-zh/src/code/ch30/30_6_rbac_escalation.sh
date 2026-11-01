# 一个只能创建 RoleBinding 的部署机器人。
kubectl -n shorty create serviceaccount shorty-deployer
kubectl apply -f role-rolebinding-creator.yaml -f rolebinding-rolebinding-creator.yaml

kubectl auth can-i create rolebindings \
  --as=system:serviceaccount:shorty:shorty-deployer -n shorty

# 它想把自己绑到 cluster-admin：创建 RoleBinding 的权利有了，但没有 cluster-admin 里的权限。
#> fails
#> expect: grant RBAC permissions|grant extra privileges
#> head: 12
kubectl -n shorty --as=system:serviceaccount:shorty:shorty-deployer create rolebinding pwn --clusterrole=cluster-admin --serviceaccount=shorty:shorty-deployer
