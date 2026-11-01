# 起点：namespace 里只有自动创建的 default。
kubectl -n shorty get serviceaccounts

kubectl -n shorty create serviceaccount shorty-api

# 1.24 之后不再为 ServiceAccount 自动创建 default-token-* Secret。
kubectl -n shorty get secrets

#> fails
kubectl -n shorty get secrets -o name | grep -c 'default-token'

# 用这个 SA 跑一个 Pod，读投射进容器的 token。
kubectl apply -f sa-token-reader.yaml
kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Succeeded pod/sa-token-reader --timeout=120s
kubectl -n shorty logs sa-token-reader

# automountServiceAccountToken: false 时连目录都不会挂进来。
kubectl apply -f sa-no-token.yaml
kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Succeeded pod/sa-no-token --timeout=120s
kubectl -n shorty logs sa-no-token

# 也可以手工签发一个短期 token，再让 apiserver 用 TokenReview 验证它。
TOKEN=$(kubectl -n shorty create token shorty-api --duration=10m)
kubectl create -f - -o jsonpath='{.status.authenticated}{"\n"}{.status.user.username}{"\n"}{.status.user.groups}{"\n"}' <<EOF
apiVersion: authentication.k8s.io/v1
kind: TokenReview
spec:
  token: $TOKEN
EOF
