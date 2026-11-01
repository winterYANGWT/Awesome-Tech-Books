kubectl -n shorty apply -f api-sidecar.yaml
kubectl -n shorty rollout status deployment/api --timeout=180s

# 滚动更新期间可能还有一个旧的、正在终止的 Pod；取创建时间最新的那个。
pod=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api \
    --sort-by=.metadata.creationTimestamp -o name | tail -n 1 | cut -d/ -f2)
echo "新的 Pod：$pod"
kubectl -n shorty get pod "$pod" -o wide

# api 仍是普通容器；metrics-scraper 在 initContainers 里，restartPolicy 是 Always。
kubectl -n shorty get pod "$pod" \
    -o jsonpath='{.spec.containers[*].name} / {.spec.initContainers[*].name}{"\n"}'

# sidecar 每 10 秒抓一次 /metrics，成功后打一行日志。等第一条成功日志出现。
set +o pipefail
#> expect: scrape ok: shorty_memory_links
timeout 40 kubectl -n shorty logs -f -c metrics-scraper "$pod" | grep -m1 'scrape ok'
set -o pipefail

kubectl -n shorty logs -c metrics-scraper "$pod" --tail=2
