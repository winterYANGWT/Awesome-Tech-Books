# 29.3 去掉 api 的 CPU requests（同时去掉 CPU limits，避免 request 从 limit 回填）。
kubectl -n shorty patch deployment api --type=json -p='[
  {"op":"replace","path":"/spec/template/spec/containers/0/resources","value":{"requests":{"memory":"64Mi"},"limits":{"memory":"256Mi"}}}
]'
#> tail: 2
kubectl -n shorty rollout status deployment/api --timeout=180s

# HPA 每 15 秒同步一次；新 Pod 的指标也要等一次采集。
# 条件先变成 FailedGetResourceMetric，Message 随后才会指出缺的是 cpu request。
for i in $(seq 1 30); do
  read -r cond msg < <(kubectl -n shorty get hpa api -o jsonpath='{range .status.conditions[?(@.type=="ScalingActive")]}{.status}/{.reason} {.message}{end}')
  echo "ScalingActive=$cond"
  case "$msg" in *"missing request for cpu"*) break;; esac
  sleep 5
done

kubectl get hpa api -n shorty

# 事件里能直接看到原因：missing request for cpu。
#> expect: missing request for cpu
kubectl -n shorty describe hpa api | sed -n '/Conditions:/,$p'

# 恢复 requests/limits，HPA 重新拿到有效指标。
kubectl -n shorty patch deployment api --type=json -p='[
  {"op":"replace","path":"/spec/template/spec/containers/0/resources","value":{"requests":{"cpu":"45m","memory":"64Mi"},"limits":{"cpu":"500m","memory":"256Mi"}}}
]'
#> tail: 2
kubectl -n shorty rollout status deployment/api --timeout=180s

for i in $(seq 1 24); do
  cond=$(kubectl -n shorty get hpa api -o jsonpath='{range .status.conditions[?(@.type=="ScalingActive")]}{.status}/{.reason}{end}')
  echo "ScalingActive=$cond"
  [ "$cond" = "True/ValidMetricFound" ] && break
  sleep 5
done
kubectl get hpa api -n shorty
