ready_endpoints() {
    kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
        -o jsonpath='{range .items[*].endpoints[*]}{.conditions.ready}{"\n"}{end}' \
        | grep -c true || true
}

kubectl -n shorty apply -f api-probes.yaml
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty get pod -l app.kubernetes.io/name=api

# 打开 not_ready 故障：进程还活着，/readyz 一直返回 503。
kubectl -n shorty set env deployment/api SHORTY_FAULT_NOT_READY=1

# 新 Pod 永远不会 Ready，Recreate 已经把旧 Pod 删掉，所以 rollout 不会完成。
#> fails
#> timeout: 60
kubectl -n shorty rollout status deployment/api --timeout=20s

kubectl -n shorty get pod -l app.kubernetes.io/name=api
echo "ready endpoints: $(ready_endpoints)"

#> expect: Readiness probe failed
kubectl -n shorty describe pod -l app.kubernetes.io/name=api | tail -n 12

# 去掉故障，readiness 恢复，EndpointSlice 重新把 Pod 算作就绪。
kubectl -n shorty set env deployment/api SHORTY_FAULT_NOT_READY-
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty get pod -l app.kubernetes.io/name=api

#> expect: ready endpoints: 1
echo "ready endpoints: $(ready_endpoints)"
