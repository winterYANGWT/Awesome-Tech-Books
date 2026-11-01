# 把 api 的两个副本都钉到 shorty-worker，shorty-worker2 上就没有本地端点。
kubectl -n shorty patch --request-timeout=300s deployment api --type=merge -p '{"spec":{"template":{"spec":{"nodeSelector":{"kubernetes.io/hostname":"shorty-worker"}}}}}'
kubectl -n shorty rollout status deployment/api --timeout=180s

# 客户端在 shorty-worker2 上。
kubectl apply --request-timeout=300s -f client-on-worker2.yaml
kubectl -n shorty wait --for=condition=Ready pod/client-local --timeout=120s

# 等 EndpointSlice 也只剩 shorty-worker 上的端点。
#> hide
#> timeout: 240
for i in $(seq 1 60); do
    nodes=$(kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
        -o jsonpath='{range .items[*]}{range .endpoints[*]}{.nodeName}{"\n"}{end}{end}' | sort -u)
    [ "$nodes" = "shorty-worker" ] && break
    sleep 1
done

# EndpointSlice 的两个地址都在 shorty-worker 上。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api -o jsonpath='{range .items[*]}{range .endpoints[*]}  {.addresses[0]} node={.nodeName}{"\n"}{end}{end}'

# 改成 Local：只允许路由到“客户端所在节点”的端点。
kubectl -n shorty patch --request-timeout=300s svc api --type=merge -p '{"spec":{"internalTrafficPolicy":"Local"}}'

# 等 kube-proxy 把新规则同步到节点（没有可等待的 condition，轮询到请求失败为止）。
#> hide
#> timeout: 300
for i in $(seq 1 60); do
    kubectl -n shorty exec client-local -- wget -q -T 2 -O /dev/null http://api:8080/healthz >/dev/null 2>&1 || break
    sleep 1
done

# shorty-worker2 上没有 api 端点，包被丢弃，表现为超时。
#> fails
#> expect: (download timed out|Connection refused|Network is unreachable)
kubectl -n shorty exec client-local -- wget -T 3 -O- http://api:8080/healthz

# 改回 Cluster：所有节点都可以路由到那两个 api 副本。
kubectl -n shorty patch --request-timeout=300s svc api --type=merge -p '{"spec":{"internalTrafficPolicy":"Cluster"}}'
#> hide
#> timeout: 300
for i in $(seq 1 60); do
    kubectl -n shorty exec client-local -- wget -q -T 2 -O /dev/null http://api:8080/healthz >/dev/null 2>&1 && break
    sleep 1
done
kubectl -n shorty exec client-local -- wget -qO- http://api:8080/healthz

# 恢复 api 的调度，并删掉只用于这次对照的客户端 Pod。
kubectl -n shorty patch --request-timeout=300s deployment api --type=json -p '[{"op":"remove","path":"/spec/template/spec/nodeSelector"}]'
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty delete pod client-local --wait=true
