# 给 shorty-api 只读 ConfigMap 的权限，不授权 Secret。
kubectl apply -f rbac.yaml
kubectl -n shorty auth can-i get configmaps --as=system:serviceaccount:shorty:shorty-api
#> fails
#> expect: ^no$
kubectl -n shorty auth can-i get secret --as=system:serviceaccount:shorty:shorty-api
kubectl -n shorty get configmap shorty-config --as=system:serviceaccount:shorty:shorty-api -o name

# 实际读 Secret 会被 API server 拒绝。
#> fails
#> expect: Forbidden
kubectl -n shorty get secret shorty-db --as=system:serviceaccount:shorty:shorty-api

# 1.24 起不再自动创建 default-token-* Secret；Pod 用的是投射的短期 token。
kubectl -n shorty get secret | grep default-token || echo '没有 default-token-* Secret'
kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{range .items[0].spec.volumes[*].projected.sources[*].serviceAccountToken}expirationSeconds={.expirationSeconds}{"\n"}{end}'

# 本实验集群的 apiserver 没有配置静态加密。
kubectl -n kube-system get pod -l component=kube-apiserver -o jsonpath='{range .items[0].spec.containers[0].command[*]}{@}{"\n"}{end}' | grep -E -- '--encryption-provider-config|--kms' || echo 'apiserver 未配置 --encryption-provider-config'
