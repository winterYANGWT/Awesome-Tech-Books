# 先问具体动作：同一个 SA 在 shorty 里能读 ConfigMap。
kubectl -n shorty auth can-i get configmaps \
  --as=system:serviceaccount:shorty:shorty-api

# 换到 default：RoleBinding 在 shorty，default 里没有绑定，答案是 no。
#> fails
kubectl -n default auth can-i get configmaps \
  --as=system:serviceaccount:shorty:shorty-api

# 角色只覆盖 configmaps，不覆盖 apps 组的 deployments。
#> fails
kubectl -n shorty auth can-i list deployments.apps \
  --as=system:serviceaccount:shorty:shorty-api

# 30.4 的 resourceNames 规则只放行叫 api 的 Deployment，大小写也要对上。
kubectl -n shorty auth can-i get deployments.apps/api \
  --as=system:serviceaccount:shorty:shorty-api
#> fails
kubectl -n shorty auth can-i get deployments.apps/API \
  --as=system:serviceaccount:shorty:shorty-api

# 再看这个身份的全部有效规则。
#> head: 25
kubectl -n shorty auth can-i --list \
  --as=system:serviceaccount:shorty:shorty-api

# Pod 模板里绑定的是哪个 SA，automount 有没有被显式关掉（空表示没写，默认挂载）。
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.serviceAccountName}{"\n"}{.spec.template.spec.automountServiceAccountToken}{"\n"}'

# RoleBinding 的 subject 必须和 Pod 用的 SA 完全一致：namespace 和名字都要对。
kubectl -n shorty get rolebinding shorty-config-reader \
  -o jsonpath='{.subjects[0].kind}/{.subjects[0].namespace}:{.subjects[0].name}{"\n"}'
