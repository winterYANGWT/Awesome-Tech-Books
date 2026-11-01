kubectl -n shorty apply -f pod_init_wait.yaml

# 等 init 容器第一次失败并被重启，Status 变成 Init:CrashLoopBackOff。
#> timeout: 120
kubectl -n shorty wait --for=jsonpath='{.status.initContainerStatuses[0].state.waiting.reason}'=CrashLoopBackOff \
    pod/api-waiter --timeout=90s
kubectl -n shorty get pod api-waiter

#> expect: Exit Code:\s+1
kubectl -n shorty describe pod api-waiter | grep -A20 'Init Containers:'

#> expect: Back-off restarting failed container
kubectl -n shorty describe pod api-waiter | sed -n '/Events:/,$p'

# DNS 的错误在 init 容器日志里，不在 Event 里。
#> expect: DNS lookup failed for api-wrong
kubectl -n shorty logs api-waiter -c wait-api

# 把名字改成真实存在的 Service api，init 成功，主容器才启动。
kubectl -n shorty apply -f pod_init_wait_ok.yaml
kubectl -n shorty wait --for=condition=Ready pod/api-waiter-ok --timeout=120s
kubectl -n shorty get pod api-waiter-ok
kubectl -n shorty delete pod api-waiter api-waiter-ok
