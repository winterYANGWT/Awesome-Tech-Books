# 脚本永远不退出，Job 会一直处于 active；没有终态条件。
kubectl apply -f job-stuck.yaml
kubectl -n shorty wait --for=condition=Ready pod -l batch.kubernetes.io/job-name=shorty-stuck --timeout=60s

# 只看 Job 的计数和条件：active=1，succeeded/failed 都为空。
kubectl -n shorty get job shorty-stuck -o custom-columns=NAME:.metadata.name,ACTIVE:.status.active,SUCCEEDED:.status.succeeded,FAILED:.status.failed
kubectl -n shorty get job shorty-stuck -o jsonpath='conditions={.status.conditions}{"\n"}'

# describe 的 Pods Statuses 说明有一个 Pod 在跑；没有 Complete/Failed。
kubectl -n shorty describe job shorty-stuck | sed -n '/^Pods Statuses:/p;/^Conditions:/,/^Events:/p'
kubectl -n shorty describe job shorty-stuck | sed -n '/^Events:/,$p' | tail -n 4

# 把 restartPolicy 改成 Always，API 直接拒绝：Job 只接受 Never/OnFailure。
#> fails
#> expect: Required value: valid values: "OnFailure", "Never"
sed -e 's/name: shorty-stuck/name: shorty-stuck-always/' -e 's/restartPolicy: Never/restartPolicy: Always/' job-stuck.yaml | kubectl apply -f -

# 清理这个永远不会完成的 Job。
kubectl -n shorty delete job shorty-stuck --wait=true
