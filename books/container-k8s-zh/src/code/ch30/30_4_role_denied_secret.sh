# api 的 SA 没有任何 secrets 权限。
#> fails
kubectl auth can-i get secrets \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

# 让 apiserver 以这个 SA 的身份做授权判断；真正拿 token 认证的路径在 30.2 的 TokenReview 里。
#> fails
#> expect: Forbidden
kubectl -n shorty --as=system:serviceaccount:shorty:shorty-api get secret shorty-db

# resourceNames 把 get 收紧到名为 api 的 Deployment；注意 apiGroups 是 apps。
kubectl apply -f role-one-deployment-reader.yaml -f rolebinding-one-deployment-reader.yaml

kubectl auth can-i get deployments.apps/api \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

#> fails
kubectl auth can-i get deployments.apps/redis \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

# 名字大小写不对也不行：can-i 是 no，真正请求是 Forbidden。
#> fails
kubectl auth can-i get deployments.apps/API \
  --as=system:serviceaccount:shorty:shorty-api -n shorty

#> fails
#> expect: Forbidden
kubectl -n shorty --as=system:serviceaccount:shorty:shorty-api get deployment redis
