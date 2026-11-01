# 给 shorty 设两条硬上限：Pod 数 10、CPU 请求合计 2 核。
kubectl apply -f quota.yaml

# 等配额控制器把 status 填上。
until [ -n "$(kubectl -n shorty get resourcequota shorty-quota -o jsonpath='{.status.hard.pods}')" ]; do sleep 1; done

#> expect: requests.cpu
# 看当前用量；Pod 数会随 HPA 和 Job 留下的 Pod 变。
kubectl -n shorty describe resourcequota shorty-quota | sed -n '/Resource/,$p'

# 再要 1800m，总量超过 2 核，准入直接拒绝。
#> fails
#> expect: exceeded quota
kubectl apply -f quota-pod.yaml

# 把 CPU 上限调到 4 核，重建同一个 Pod。
kubectl -n shorty patch resourcequota shorty-quota --type=merge \
  -p '{"spec":{"hard":{"requests.cpu":"4"}}}'
kubectl apply -f quota-pod.yaml
kubectl -n shorty wait --for=condition=Ready pod/quota-filler --timeout=120s

# LimitRange 给没写 requests/limits 的容器补默认值，避免新 Pod 被配额拒绝。
kubectl apply -f limitrange.yaml
kubectl apply -f defaulted-pod.yaml
kubectl -n shorty wait --for=condition=Ready pod/defaulted-pod --timeout=120s

#> expect: "cpu":"50m"
#> expect: "cpu":"200m"
kubectl -n shorty get pod defaulted-pod \
  -o jsonpath='{.spec.containers[0].resources}{"\n"}'

#> expect: 4
# 硬上限已经是 4 核；状态里的用量会异步更新，这里读 spec 确认改成功了。
kubectl -n shorty get resourcequota shorty-quota -o jsonpath='{.spec.hard}{"\n"}'
