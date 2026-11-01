# 29.1 安装 metrics-server，观察 Metrics API 从无到有。
# 此时集群里还没有 Metrics API：top 和 HPA 都拿不到数据。
#> fails
#> expect: Metrics API not available
kubectl top pods -n shorty

# kind 默认不带 metrics-server，先看 APIService 不存在。
#> fails
kubectl get apiservice v1beta1.metrics.k8s.io

# 官方清单注册 metrics.k8s.io；kind 的 kubelet 是自签证书，要加 --kubelet-insecure-tls。
kubectl apply -f metrics-server-components.yaml

kubectl -n kube-system patch deployment metrics-server --type=json \
  -p='[{"op":"add","path":"/spec/template/spec/containers/0/args/-","value":"--kubelet-insecure-tls"}]'

#> tail: 2
kubectl -n kube-system rollout status deployment/metrics-server --timeout=180s

kubectl wait --for=condition=Available apiservice/v1beta1.metrics.k8s.io --timeout=120s

# APIService 可用不代表第一次采集已经到了，轮询到 top 能读出数据为止。
#> timeout: 240
until kubectl top nodes >/dev/null 2>&1; do sleep 2; done

kubectl top nodes
kubectl top pods -n shorty

# 删掉整个 metrics-server，APIService 一起消失。
kubectl delete -f metrics-server-components.yaml
kubectl wait --for=delete apiservice/v1beta1.metrics.k8s.io --timeout=120s

# 没有 Metrics API 时，top 报的就是这个错误，不是 Pod 或节点坏了。
#> fails
#> expect: Metrics API not available
kubectl top pods -n shorty

# 装回来，后面的 HPA 和 VPA 都要用。
kubectl apply -f metrics-server-components.yaml
kubectl -n kube-system patch deployment metrics-server --type=json \
  -p='[{"op":"add","path":"/spec/template/spec/containers/0/args/-","value":"--kubelet-insecure-tls"}]'
#> tail: 2
kubectl -n kube-system rollout status deployment/metrics-server --timeout=180s
kubectl wait --for=condition=Available apiservice/v1beta1.metrics.k8s.io --timeout=120s
#> timeout: 240
until kubectl top nodes >/dev/null 2>&1; do sleep 2; done
kubectl get apiservice v1beta1.metrics.k8s.io
