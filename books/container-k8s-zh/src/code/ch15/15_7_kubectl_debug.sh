# 15.1-15.5 留下的容器和 builder 先清掉，单节点 kind 才有内存。
docker ps -aq --filter label=devcontainer.local_folder | xargs -r docker rm -f >/dev/null
docker ps -aq --filter label=org.testcontainers=true | xargs -r docker rm -f >/dev/null
docker buildx rm -f ch15ci >/dev/null 2>&1 || true

source ../lib/common.sh
cluster_up kind-1n.yaml
kind load docker-image --name shorty shorty:0.1 >/dev/null
echo 'shorty:0.1 loaded'

kubectl apply -f 00-namespace.yaml -f shorty-pod.yaml -f shorty-service.yaml
kubectl -n shorty wait --for=condition=Ready pod/api --timeout=120s
kubectl -n shorty get pod api -o wide

(
  set -e
  kubectl -n shorty port-forward service/api 18080:8080 >/tmp/ch15-pf.log 2>&1 &
  pf=$!
  trap 'kill $pf 2>/dev/null || true' EXIT
  for _ in $(seq 1 40); do
    curl -fsS http://localhost:18080/healthz >/tmp/ch15-health 2>/dev/null && break
    sleep 1
  done
  cat /tmp/ch15-health
  grep 'Forwarding from' /tmp/ch15-pf.log
  kill "$pf"
  wait "$pf" 2>/dev/null || true
  trap - EXIT
)

(
  set -e
  uid_before=$(kubectl -n shorty get pod api -o jsonpath='{.metadata.uid}')
  kubectl -n shorty debug pod/api --image=busybox:1.37 --target=api \
    -c debugger --attach=false -- sleep 3600
  for _ in $(seq 1 60); do
    [ -n "$(kubectl -n shorty get pod api \
      -o jsonpath='{.status.ephemeralContainerStatuses[0].state.running.startedAt}')" ] && break
    sleep 1
  done
  kubectl -n shorty exec api -c debugger -- ps > /tmp/ch15-ps
  cat /tmp/ch15-ps
  grep -q '[s]horty' /tmp/ch15-ps && echo 'shorty process visible'
  kubectl -n shorty exec api -c debugger -- wget -qO- http://127.0.0.1:8080/healthz
  uid_after=$(kubectl -n shorty get pod api -o jsonpath='{.metadata.uid}')
  restarts=$(kubectl -n shorty get pod api \
    -o jsonpath='{.status.containerStatuses[0].restartCount}')
  [ "$uid_before" = "$uid_after" ] && echo 'pod uid unchanged'
  echo "restartCount=$restarts"
)
