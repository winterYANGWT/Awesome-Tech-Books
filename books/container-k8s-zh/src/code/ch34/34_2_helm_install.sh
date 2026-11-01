# chart 文件：chart/Chart.yaml、chart/values.yaml、chart/templates/_helpers.tpl、
# chart/templates/deployment.yaml、chart/templates/service.yaml、chart/templates/NOTES.txt。
# 34.2 Helm：先在本地渲染，确认 values 生效，再安装 release。
helm lint ./chart

helm template shorty ./chart -n shorty-helm --set replicaCount=3 \
    | yq eval-all -r 'select(.kind == "Deployment") | "replicas=\(.spec.replicas) image=\(.spec.template.spec.containers[0].image)"'

#> expect: STATUS: deployed
helm install shorty ./chart -n shorty-helm --create-namespace --wait --timeout 180s

kubectl -n shorty-helm get deploy,svc,pods -o wide

# release 记录存在 Secret 里，类型是 helm.sh/release.v1。
kubectl -n shorty-helm get secret -l owner=helm \
    -o custom-columns=NAME:.metadata.name,TYPE:.type

helm history shorty -n shorty-helm

# 把副本数从 2 改到 3：template 只渲染本地，diff 拿它和集群里的对象比。
helm template shorty ./chart -n shorty-helm --set replicaCount=3 \
    | kubectl -n shorty-helm diff -f - || true

helm upgrade shorty ./chart -n shorty-helm --set replicaCount=3 --wait --timeout 180s
kubectl -n shorty-helm get deploy shorty -o jsonpath='{.spec.replicas}{"\n"}'

helm rollback shorty 1 -n shorty-helm --wait --timeout 180s
kubectl -n shorty-helm get deploy shorty -o jsonpath='{.spec.replicas}{"\n"}'
helm history shorty -n shorty-helm

# chart 也能作为 OCI artifact 推 registry；这里推到本章的本地 registry。
helm package ./chart --destination /tmp >/dev/null
helm push /tmp/shorty-0.1.0.tgz oci://localhost:5000/charts --plain-http
helm show chart oci://localhost:5000/charts/shorty --version 0.1.0 --plain-http
