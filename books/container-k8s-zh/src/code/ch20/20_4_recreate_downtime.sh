kubectl -n shorty patch deployment api --type=json -p='[{"op":"replace","path":"/spec/strategy/type","value":"Recreate"},{"op":"remove","path":"/spec/strategy/rollingUpdate"}]'
kubectl -n shorty get deployment api -o jsonpath='strategy={.spec.strategy.type}{"\n"}'
kubectl -n shorty exec client -- sh -c 'rm -f /tmp/stop; i=0; while [ ! -f /tmp/stop ]; do if wget -q -O /dev/null -T 1 http://api:8080/healthz; then echo ok; else echo fail; fi; i=$((i+1)); sleep 0.1; done; echo "requests=$i"' > /tmp/ch20_recreate.log 2>&1 &
LOOP=$!
kubectl -n shorty set image deployment/api api=shorty:0.1
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty exec client -- touch /tmp/stop
wait "$LOOP"
#> expect: ok=\d+ fail=[1-9][0-9]*
awk '/^ok$/{ok++} /^fail$/{fail++} END{printf "ok=%d fail=%d\n", ok, fail}' /tmp/ch20_recreate.log
kubectl -n shorty get pods -l app.kubernetes.io/name=api
# 演练结束，恢复 RollingUpdate，后面 PDB 的例子里 api 还要继续用。
kubectl -n shorty patch deployment api --type=json -p='[{"op":"replace","path":"/spec/strategy/type","value":"RollingUpdate"},{"op":"add","path":"/spec/strategy/rollingUpdate","value":{"maxSurge":1,"maxUnavailable":1}}]'
kubectl -n shorty get deployment api -o jsonpath='strategy={.spec.strategy.type} maxSurge={.spec.strategy.rollingUpdate.maxSurge} maxUnavailable={.spec.strategy.rollingUpdate.maxUnavailable}{"\n"}'
