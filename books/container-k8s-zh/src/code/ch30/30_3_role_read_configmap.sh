# Role 只给 shorty 这个 namespace 里的 configmaps；RoleBinding 把角色绑到 SA。
kubectl apply -f role-config-reader.yaml -f rolebinding-config-reader.yaml

kubectl -n shorty describe role shorty-config-reader

# 换成 --as 的 SA 身份，问一句能不能读 ConfigMap。
kubectl auth can-i get configmaps \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

kubectl auth can-i list configmaps \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

# 这个 SA 的有效规则：除了 configmaps，还有所有已认证身份都有的 selfsubject* 和 discovery。
kubectl auth can-i --list \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

# 让 api 的 Pod 真正用上这个 SA。
kubectl -n shorty patch deployment api --type=strategic \
  -p '{"spec":{"template":{"spec":{"serviceAccountName":"shorty-api"}}}}'
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty get deployment api \
  -o jsonpath='{.spec.template.spec.serviceAccountName}{"\n"}'
