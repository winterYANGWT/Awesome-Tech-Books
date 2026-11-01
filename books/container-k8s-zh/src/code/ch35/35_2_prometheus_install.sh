# 35.2 用 prometheus-operator 的 CRD 装一个单副本 Prometheus。
# setup.sh 已经加好 helm 仓库、把镜像加载进节点；这里只做安装和观察。

# 先确认 metrics-server 只提供资源指标：top 能用，但它没有历史，也不是 Prometheus。
kubectl get apiservice v1beta1.metrics.k8s.io
for i in $(seq 1 60); do
  kubectl top nodes >/dev/null 2>&1 && break
  sleep 1
done
kubectl top pods -n shorty

# kube-prometheus-stack 打包了 operator、Prometheus、Alertmanager、Grafana 和 node-exporter；
# values 文件关掉后面几样，只留一个 Prometheus 和 kube-state-metrics。
#> head: 8
helm upgrade --install kps prometheus-community/kube-prometheus-stack \
  --version 92.14.1 -n monitoring --create-namespace \
  -f prometheus-values.yaml --wait --timeout 600s

# Helm 的 --wait 只等它认识的原生对象；Prometheus 的 StatefulSet 是 operator 稍后创建的。
until kubectl -n monitoring get statefulset prometheus-kps-kube-prometheus-stack-prometheus >/dev/null 2>&1; do
  sleep 1
done
kubectl -n monitoring rollout status \
  statefulset/prometheus-kps-kube-prometheus-stack-prometheus --timeout=180s

# operator 从 Prometheus 这个 CR 生成 StatefulSet、ConfigMap 和 Service。
kubectl get crd | grep monitoring.coreos.com
kubectl -n monitoring get prometheus
kubectl -n monitoring get prometheus kps-kube-prometheus-stack-prometheus \
  -o jsonpath='{.spec.replicas} replica, retention {.spec.retention}{"\n"}'

# 只剩 operator、kube-state-metrics 和 Prometheus：没有 Grafana、Alertmanager、node-exporter。
kubectl -n monitoring get deploy,statefulset,daemonset
kubectl -n monitoring get pods

# Prometheus CR 只选带 release=kps 标签的 ServiceMonitor，下一节的 shorty 要带上这个标签。
kubectl -n monitoring get prometheus kps-kube-prometheus-stack-prometheus \
  -o jsonpath='{.spec.serviceMonitorSelector}{"\n"}{.spec.serviceMonitorNamespaceSelector}{"\n"}'
kubectl -n monitoring get servicemonitor
