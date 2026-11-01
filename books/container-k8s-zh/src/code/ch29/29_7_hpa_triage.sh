# 29.7 一次按顺序的 HPA 排查。
# 1. Metrics API 在不在：Available 为 True，SERVICE 指向 kube-system/metrics-server。
kubectl get apiservice v1beta1.metrics.k8s.io

# 2. 指标能不能读出来：top 有数据，说明 metrics-server 到 kubelet 的路是通的。
kubectl top pods -n shorty -l app.kubernetes.io/name=api

# 3. HPA 自己的状态：TARGETS 是“当前利用率/目标”，REPLICAS 是实际副本数。
kubectl get hpa api -n shorty

# 4. Conditions 说明能不能算、有没有被上下限截断；Events 保留最近的失败原因。
#> tail: 30
kubectl -n shorty describe hpa api | sed -n '/Conditions:/,$p'

# 5. 最后回到被 HPA 的 Deployment，确认 requests 就是利用率的分母。
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.containers[0].resources}{"\n"}'
