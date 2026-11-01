# 16.5 把镜像装进节点，apply 第一个 Deployment 和 ClusterIP Service。
kind load docker-image --name shorty shorty:0.1
kubectl apply -f namespace.yaml -f api-deployment.yaml -f api-service.yaml
kubectl -n shorty rollout status deployment/api --timeout=120s
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide
kubectl -n shorty get service api -o wide
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=api

# port-forward 只把 Service 的端口临时映射到本地；它不是 Service 的一部分。
kubectl -n shorty port-forward service/api 18080:8080 >/tmp/ch16-port-forward.log 2>&1 &
pf_pid=$!
cleanup() {
  kill "$pf_pid" 2>/dev/null || true
  wait "$pf_pid" 2>/dev/null || true
}
trap cleanup EXIT
curl -fs --retry 30 --retry-all-errors --retry-delay 1 http://127.0.0.1:18080/healthz
curl -fsS http://127.0.0.1:18080/ | grep -o '<title>[^<]*</title>'
cleanup
trap - EXIT

kubectl -n shorty get deployment api \
  -o jsonpath='desired={.spec.replicas} ready={.status.readyReplicas}{"\n"}'
