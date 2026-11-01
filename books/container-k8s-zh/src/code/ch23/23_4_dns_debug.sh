# CoreDNS 是一个 Deployment，前面挂一个名叫 kube-dns 的 ClusterIP Service。
kubectl -n kube-system get deployment coredns
kubectl -n kube-system get svc kube-dns

# busybox 自带 nslookup，用它看 Pod 实际拿到的 resolv.conf。
kubectl -n shorty run dnsutils --image=busybox:1.37 --image-pull-policy=IfNotPresent --restart=Never --command -- sleep 3600
kubectl -n shorty wait --for=condition=Ready pod/dnsutils --timeout=120s
kubectl -n shorty exec dnsutils -- cat /etc/resolv.conf

# 完整域名：<service>.<namespace>.svc.cluster.local，解析到 ClusterIP。
kubectl -n shorty exec dnsutils -- nslookup api.shorty.svc.cluster.local

# 同一个 namespace 里可以只用短名；search 域会把它补全。
# 原始输出还会尝试 search 里的宿主机域并打印 NXDOMAIN/SERVFAIL，这里只取解析结果。
kubectl -n shorty exec dnsutils -- nslookup api 2>&1 | grep -A1 '^Name:' || true

# 不存在的名字返回 NXDOMAIN。
#> fails
#> expect: NXDOMAIN
kubectl -n shorty exec dnsutils -- nslookup api.nope.svc.cluster.local

# 调试 Pod 用完就删，不给后面的章节留噪音。
kubectl -n shorty delete pod dnsutils --wait=true
