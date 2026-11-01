# 一个 selector 写错、没有后端的 Service。
kubectl apply --request-timeout=300s -f api-broken.yaml
kubectl -n shorty get svc api-broken

# 新写法：先看 EndpointSlice 有没有 ready 端点。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api-broken
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api-broken -o jsonpath='{range .items[*]}endpoints={.endpoints}{"\n"}{end}'

# 对照：Pod 本身是 Running 的，只是标签不匹配。
kubectl -n shorty get pods -l app.kubernetes.io/name=api

# 没有端点时，kube-proxy 对 ClusterIP 的 TCP 连接直接拒绝。
#> fails
#> expect: Connection refused
kubectl -n shorty exec client -- wget -T 3 -O- http://api-broken:8080/healthz

# 旧的 Endpoints API 仍能看，但只作为兼容入口。
kubectl -n shorty get endpoints api-broken

# 修正 selector 后，EndpointSlice 重新出现，连接恢复。
kubectl -n shorty patch --request-timeout=300s svc api-broken --type=merge -p '{"spec":{"selector":{"app.kubernetes.io/name":"api"}}}'
#> hide
for i in $(seq 1 30); do
    n=$(kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api-broken \
        -o jsonpath='{range .items[*]}{range .endpoints[?(@.conditions.ready==true)]}{.addresses[0]}{"\n"}{end}{end}' | wc -l)
    [ "$n" -ge 1 ] && break
    sleep 1
done
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api-broken
kubectl -n shorty exec client -- wget -qO- http://api-broken:8080/healthz

# 关掉 setup.sh 启动的 LoadBalancer 控制器，不给下一个实验留后台进程。
#> hide
pkill -f /usr/local/bin/cloud-provider-kind || true
