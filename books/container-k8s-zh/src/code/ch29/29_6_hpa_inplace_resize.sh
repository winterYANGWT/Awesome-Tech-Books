# 29.6 原地调整：直接改一个 api Pod 的 CPU limit，不经过 Deployment 滚动更新。
pod=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{.items[0].metadata.name}')
before=$(kubectl -n shorty get pod "$pod" -o jsonpath='{.status.containerStatuses[0].restartCount}')
echo "pod=$pod restartCount=$before"

# Pod 的 spec.containers[*].resources 是可变的；resize 子资源把新值交给 kubelet。
kubectl -n shorty patch pod "$pod" --subresource resize --type=strategic \
  -p='{"spec":{"containers":[{"name":"api","resources":{"limits":{"cpu":"600m"}}}]}}'

for i in $(seq 1 30); do
  status_cpu=$(kubectl -n shorty get pod "$pod" -o jsonpath='{.status.containerStatuses[0].resources.limits.cpu}')
  echo "status.limits.cpu=$status_cpu"
  [ "$status_cpu" = "600m" ] && break
  sleep 2
done

after=$(kubectl -n shorty get pod "$pod" -o jsonpath='{.status.containerStatuses[0].restartCount}')
echo "restartCount before=$before after=$after"
kubectl -n shorty describe pod "$pod" | sed -n '/Containers:/,/Conditions:/p'

# 再要 1000 个 CPU：超过节点 allocatable，apiserver 直接拒绝这次 resize。
#> fails
#> expect: node didn't have enough allocatable resources
kubectl -n shorty patch pod "$pod" --subresource resize --type=strategic \
  -p='{"spec":{"containers":[{"name":"api","resources":{"requests":{"cpu":"1000"},"limits":{"cpu":"1000"}}}]}}'

# 请求被拒，Pod 的 spec 和 status 都还是上一次的 600m。
kubectl -n shorty get pod "$pod" -o jsonpath='{.spec.containers[0].resources}{"\n"}'
kubectl -n shorty get pod "$pod" -o jsonpath='{range .status.conditions[*]}{.type}={.status} {.reason}{"\n"}{end}'
kubectl -n shorty get pod "$pod" -o jsonpath='{.status.containerStatuses[0].resources}{"\n"}'

# 把请求改回可行值，再观察一次成功的 resize。
kubectl -n shorty patch pod "$pod" --subresource resize --type=strategic \
  -p='{"spec":{"containers":[{"name":"api","resources":{"requests":{"cpu":"45m"},"limits":{"cpu":"500m"}}}]}}'
for i in $(seq 1 30); do
  status_cpu=$(kubectl -n shorty get pod "$pod" -o jsonpath='{.status.containerStatuses[0].resources.limits.cpu}')
  echo "status.limits.cpu=$status_cpu"
  [ "$status_cpu" = "500m" ] && break
  sleep 2
done
kubectl -n shorty get pod "$pod"
