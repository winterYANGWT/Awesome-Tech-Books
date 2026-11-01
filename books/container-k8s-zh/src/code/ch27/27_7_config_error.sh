kubectl apply -f error-pods.yaml

# 两个 Pod 都卡在 CreateContainerConfigError。
kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].state.waiting.reason}'=CreateContainerConfigError pod/missing-secret --timeout=120s
kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].state.waiting.reason}'=CreateContainerConfigError pod/wrong-key --timeout=120s
kubectl -n shorty get pod missing-secret wrong-key

# describe 的 Event 给出具体原因：Secret 不存在，或 key 不存在。
kubectl -n shorty describe pod missing-secret | grep -E 'Error:|Failed' | tail -n 2
kubectl -n shorty describe pod wrong-key | grep -E "couldn't find key|Error:" | tail -n 2
kubectl -n shorty get secret shorty-db-wrongkey -o jsonpath='{.data}'

# 补上 Secret 和缺失的 key，两个 Pod 都能起来。
kubectl -n shorty create secret generic shorty-db-missing --from-literal=db_password=demo
kubectl -n shorty wait --for=condition=Ready pod/missing-secret --timeout=120s
kubectl -n shorty patch secret shorty-db-wrongkey --type=merge -p '{"data":{"db_password":"ZGVtbw=="}}'
kubectl -n shorty wait --for=condition=Ready pod/wrong-key --timeout=120s

# 非 root 读 0440 的 ConfigMap 文件：有 fsGroup 时组匹配，能读。
kubectl -n shorty wait --for=condition=Ready pod/perm-check pod/perm-no-fsgroup --timeout=120s
kubectl -n shorty exec perm-check -- sh -c 'id; ls -lL /etc/perm/app.conf; cat /etc/perm/app.conf'

# 没有 fsGroup 时文件属组是 root，65532 读不了。
#> fails
#> expect: Permission denied
kubectl -n shorty exec perm-no-fsgroup -- cat /etc/perm/app.conf
