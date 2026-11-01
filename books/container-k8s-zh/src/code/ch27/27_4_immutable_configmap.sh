# 建一个 immutable 的 ConfigMap，Deployment 用 checksum/config 注解记录版本。
kubectl apply -f immutable-config.yaml
kubectl -n shorty rollout status deployment/release-demo --timeout=180s
kubectl -n shorty get configmap shorty-release -o jsonpath='{.immutable}{"\n"}{.data.RELEASE}{"\n"}'
kubectl -n shorty exec deployment/release-demo -- printenv RELEASE

# 直接改 immutable 对象，API server 拒绝。
#> fails
#> expect: field is immutable
kubectl -n shorty patch configmap shorty-release --type=merge -p '{"data":{"RELEASE":"green"}}'

# 只能删了重建；再改 Deployment 的 checksum 注解，触发滚动更新。
kubectl -n shorty delete configmap shorty-release --wait=true
kubectl apply -f immutable-config-v2.yaml

# 用新名字和新的 checksum 改 Pod 模板，Deployment 滚动到新对象。
kubectl -n shorty patch deployment release-demo --type=json -p '[
  {"op":"add","path":"/spec/template/metadata/labels/release","value":"green"},
  {"op":"add","path":"/spec/template/metadata/annotations/checksum~1config","value":"green"},
  {"op":"replace","path":"/spec/template/spec/containers/0/envFrom","value":[{"configMapRef":{"name":"shorty-release-v2"}}]}
]' 
kubectl -n shorty rollout status deployment/release-demo --timeout=180s

# 新 Pod 拿到新值。
POD=$(kubectl -n shorty get pods -l release=green -o jsonpath='{.items[0].metadata.name}')
kubectl -n shorty exec "$POD" -- printenv RELEASE
kubectl -n shorty get pods -l release=green -o custom-columns=NAME:.metadata.name,PHASE:.status.phase,READY:.status.containerStatuses[0].ready
