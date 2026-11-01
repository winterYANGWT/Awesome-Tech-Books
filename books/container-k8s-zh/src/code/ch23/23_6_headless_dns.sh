# postgres 的 Service 是 headless：clusterIP 为 None，没有虚拟 IP。
kubectl -n shorty get svc postgres -o custom-columns=NAME:.metadata.name,CLUSTERIP:.spec.clusterIP,IPFAMILIES:.spec.ipFamilies,IPFAMILYPOLICY:.spec.ipFamilyPolicy
kubectl -n shorty get svc -o wide

# 在 shorty 命名空间里起客户端，查 StatefulSet 的稳定域名。
kubectl -n shorty run dnsutils --image=busybox:1.37 --image-pull-policy=IfNotPresent --restart=Never --command -- sleep 3600
kubectl -n shorty wait --for=condition=Ready pod/dnsutils --timeout=120s

# 完整域名末尾加点，跳过 search 列表，直接问集群 DNS。
kubectl -n shorty exec dnsutils -- nslookup postgres-0.postgres.shorty.svc.cluster.local.

# 返回的地址就是 postgres-0 的 Pod IP，不是某个 Service 虚拟 IP。
kubectl -n shorty get pod postgres-0 -o wide

# 查 headless Service 名本身；过滤掉 search 其他候选域的 NXDOMAIN/SERVFAIL。
kubectl -n shorty exec dnsutils -- nslookup postgres 2>&1 | grep -A1 '^Name:' || true

# 清理调试 Pod。
kubectl -n shorty delete pod dnsutils --wait=true
