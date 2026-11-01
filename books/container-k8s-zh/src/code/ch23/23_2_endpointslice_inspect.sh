# EndpointSlice 用 label 关联 Service，名字一般是 Service 名加一段哈希。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api

# 展开每个 endpoint：地址、ready、所在节点；和 Pod 列表对照。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api -o jsonpath='{range .items[*]}{.metadata.name}{" port="}{.ports[0].port}{"\n"}{range .endpoints[*]}  {.addresses[0]} ready={.conditions.ready} node={.nodeName}{"\n"}{end}{end}'
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide

# 删掉当前的一个 api Pod，让控制器重建；记下旧 IP，稍后确认它从 slice 里消失。
old_pod=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{.items[0].metadata.name}')
old_ip=$(kubectl -n shorty get pod "$old_pod" -o jsonpath='{.status.podIP}')
printf 'delete pod=%s ip=%s\n' "$old_pod" "$old_ip"
kubectl -n shorty delete pod "$old_pod" --wait=true
kubectl -n shorty rollout status deployment/api --timeout=120s

# 等 slice 里的地址与当前 Ready Pod 的 IP 完全一致（终止中的旧端点可能短暂保留）。
#> hide
for i in $(seq 1 60); do
    slice_ips=$(kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api \
        -o jsonpath='{range .items[*]}{range .endpoints[?(@.conditions.ready==true)]}{.addresses[0]}{"\n"}{end}{end}' | sort)
    pod_ips=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api \
        -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready")].status=="True")]}{.status.podIP}{"\n"}{end}' | sort)
    [ -n "$slice_ips" ] && [ "$slice_ips" = "$pod_ips" ] && break
    sleep 1
done

# 旧 IP 不在了，新 Pod 的 IP 出现在 slice 里。
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api -o jsonpath='{range .items[*]}{range .endpoints[*]}  {.addresses[0]} ready={.conditions.ready} node={.nodeName}{"\n"}{end}{end}'
printf 'old=%s still-in-slice=' "$old_ip"
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api -o jsonpath='{range .items[*]}{range .endpoints[*]}{.addresses[0]}{"\n"}{end}{end}' | grep -qx "$old_ip" && echo yes || echo no

# Endpoints 是同一份数据的旧 API，仍在，但新代码和排错都应以 EndpointSlice 为准。
kubectl -n shorty get endpoints api
