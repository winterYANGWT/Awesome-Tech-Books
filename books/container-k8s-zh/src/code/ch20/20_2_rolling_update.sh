kubectl -n shorty scale deployment api --replicas=2
kubectl -n shorty wait --for=jsonpath='{.status.readyReplicas}'=2 deployment/api --timeout=120s
kubectl -n shorty get deployment api
kubectl -n shorty get deployment api -o jsonpath='strategy={.spec.strategy.type} maxSurge={.spec.strategy.rollingUpdate.maxSurge} maxUnavailable={.spec.strategy.rollingUpdate.maxUnavailable} minReadySeconds={.spec.minReadySeconds} progressDeadlineSeconds={.spec.progressDeadlineSeconds}{"\n"}'
# 客户端 Pod 用 Service 名字连续请求，统计滚动更新期间的失败次数。
kubectl -n shorty run client --image=busybox:1.37 --image-pull-policy=IfNotPresent --restart=Never --command -- sleep 3600
kubectl -n shorty wait --for=condition=Ready pod/client --timeout=120s
OLDHASH=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{.items[0].metadata.labels.pod-template-hash}')
echo "OLDHASH=$OLDHASH"
kubectl -n shorty exec client -- sh -c 'rm -f /tmp/stop; i=0; while [ ! -f /tmp/stop ]; do if wget -q -O /dev/null -T 1 http://api:8080/healthz; then echo ok; else echo fail; fi; i=$((i+1)); sleep 0.1; done; echo "requests=$i"' > /tmp/ch20_rolling.log 2>&1 &
LOOP=$!
kubectl -n shorty annotate deployment/api kubernetes.io/change-cause="rolling update to shorty:0.2" --overwrite
kubectl -n shorty set image deployment/api api=shorty:0.2
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty exec client -- touch /tmp/stop
wait "$LOOP"
#> expect: ok=\d+ fail=0
awk '/^ok$/{ok++} /^fail$/{fail++} END{printf "ok=%d fail=%d\n", ok, fail}' /tmp/ch20_rolling.log
kubectl -n shorty wait --for=delete pod -l pod-template-hash="$OLDHASH" --timeout=60s
kubectl -n shorty get rs
kubectl -n shorty get pods -l app.kubernetes.io/name=api --show-labels
kubectl -n shorty describe deployment api | sed -n '/^Events:/,$p'
