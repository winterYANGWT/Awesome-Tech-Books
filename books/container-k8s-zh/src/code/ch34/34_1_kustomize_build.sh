# 本章的 Kustomize 清单：base 的 kustomization.yaml、deployment.yaml、service.yaml；
# overlays/dev 和 overlays/prod 的 kustomization.yaml、resources.yaml；
# commonlabels/deployment.yaml、badpatch/replicas.yaml、goodpatch/replicas.yaml。
# 34.1 base + overlay：只渲染，不碰集群。
kubectl kustomize kustomize/overlays/prod | yq eval-all -r 'select(.kind == "Deployment") | "deployment=\(.metadata.name) replicas=\(.spec.replicas) image=\(.spec.template.spec.containers[0].image) cpu=\(.spec.template.spec.containers[0].resources.requests.cpu) memory=\(.spec.template.spec.containers[0].resources.requests.memory)"'

# configMapGenerator 生成的名字带内容 hash；Deployment 里的 envFrom 已被改写。
kubectl kustomize kustomize/overlays/prod | yq eval-all -r 'select(.kind == "ConfigMap") | "configmap=\(.metadata.name) baseURL=\(.data.SHORTY_BASE_URL)"'

# 把两边的生成名归一化，再 diff；剩下的差异应该只有副本数、域名和资源。
for env in dev prod; do
    kubectl kustomize "kustomize/overlays/$env" \
        | yq 'select(.kind == "Deployment" or .kind == "ConfigMap")' \
        | sed -E 's/shorty-runtime-[a-z0-9]+/shorty-runtime-HASH/g' \
        > "/tmp/ch34-$env.yaml"
done
#> head: 70
diff -u /tmp/ch34-dev.yaml /tmp/ch34-prod.yaml || true

# apply -k 才写集群；--dry-run=server 让 apiserver 校验，不落盘。
kubectl apply -k kustomize/overlays/prod --dry-run=server

# commonLabels 会把标签同时写进 selector。对已存在的 Deployment，这等于改 selector。
kubectl kustomize kustomize/commonlabels | yq eval-all -r 'select(.kind == "Deployment") | "selector=\(.spec.selector.matchLabels)"'

#> fails
#> tail: 3
#> expect: immutable
kubectl kustomize kustomize/commonlabels | kubectl apply --dry-run=server -f -
