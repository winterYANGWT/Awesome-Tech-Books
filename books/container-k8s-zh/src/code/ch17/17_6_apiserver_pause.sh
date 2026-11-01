#!/usr/bin/env bash

#> hide
POD_UID=$(kubectl get pod -n shorty -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.uid}')

#> expect: shorty-control-plane
docker pause shorty-control-plane

# 暂停期间节点整体冻结；先等 10 秒，再用 5 秒超时的请求观察失败。
#> hide
sleep 10

#> fails
#> timeout: 60
#> expect: Unable to connect to the server
#> expect: Unable to connect to the server: .*(context deadline exceeded|Client\.Timeout exceeded)
timeout 20 kubectl --request-timeout=5s get pods -n shorty

docker unpause shorty-control-plane

kubectl wait --for=condition=Ready pod -n shorty \
  -l app.kubernetes.io/name=api --timeout=120s
kubectl get pods -n shorty

[ "$POD_UID" = "$(kubectl get pod -n shorty -l app.kubernetes.io/name=api \
  -o jsonpath='{.items[0].metadata.uid}')" ] \
  && echo "pause 前后 Pod UID 相同：容器没有被重建"
