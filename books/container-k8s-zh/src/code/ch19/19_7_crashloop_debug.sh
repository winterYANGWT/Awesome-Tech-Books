kubectl -n shorty apply -f pod_crash.yaml

#> timeout: 180
kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].state.waiting.reason}'=CrashLoopBackOff \
    pod/api-crash --timeout=120s
kubectl -n shorty get pod api-crash

# describe 里的 Last State 说明上一次为什么退出；Exit Code 1 是应用自己退出的。
#> expect: Exit Code:\s+1
kubectl -n shorty describe pod api-crash | grep -A4 'Last State'

# 容器已经退出、正在退避等待；--previous 才看得到上一次实例的日志。
#> expect: 故障开关触发退出
kubectl -n shorty logs api-crash --previous

kubectl -n shorty delete pod api-crash
