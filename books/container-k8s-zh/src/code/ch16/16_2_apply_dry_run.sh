# 16.2 先看对象里的 spec 和 status 分别由谁写。
kubectl -n shorty get deployment api -o json \
  | jq '{spec_replicas: .spec.replicas, status_replicas: .status.replicas, ready: .status.readyReplicas, available: .status.availableReplicas}'

kubectl -n shorty get deployment api --show-managed-fields -o json \
  | jq -r '[.metadata.managedFields[] | .manager + " " + .operation] | unique[]'

kubectl -n shorty get deployment api -o json \
  | jq -r '.metadata.annotations["kubectl.kubernetes.io/last-applied-configuration"] | fromjson | {kind, name: .metadata.name, replicas: .spec.replicas}'

# --dry-run=server 把对象送到 apiserver 做准入和字段校验，但不持久化。
kubectl apply --dry-run=server -f api-2-replicas.yaml

# 同一个对象已经 apply 过，kubectl diff 没有差异时退出 0。
kubectl diff -f api-2-replicas.yaml
echo "无差异时 kubectl diff 的退出码：$?"

# 把副本数改成 3 再做 diff；diff 有差异时退出 1，这是它的约定，不是脚本坏了。
sed 's/^  replicas: 2$/  replicas: 3/' api-2-replicas.yaml > /tmp/ch16-api-3.yaml
#> fails
#> expect: replicas: 3
kubectl diff -f /tmp/ch16-api-3.yaml

# 未知字段会在服务端严格解码时报错，不会被静默丢掉。
#> fails
#> expect: strict decoding error: unknown field "spec.replicasCount"
kubectl apply --dry-run=server -f api-bad-field.yaml
