# 29.5 只装 VPA Recommender：没有 Updater 和 Admission Controller，不会改 Pod。
kubectl apply -f vpa-components.yaml
kubectl wait --for=condition=Established crd/verticalpodautoscalers.autoscaling.k8s.io --timeout=120s
kubectl -n kube-system rollout status deployment/vpa-recommender --timeout=180s

# updateMode: Off 只把推荐写进 status，不动 Deployment 的模板。
kubectl apply -f vpa-api.yaml
kubectl -n shorty get vpa api-vpa

#> timeout: 360
for i in $(seq 1 36); do
  cpu=$(kubectl -n shorty get vpa api-vpa -o jsonpath='{.status.recommendation.containerRecommendations[0].target.cpu}')
  echo "recommendation target.cpu=${cpu:-pending}"
  [ -n "$cpu" ] && break
  sleep 10
done

kubectl -n shorty get vpa api-vpa
kubectl -n shorty describe vpa api-vpa | sed -n '/Recommendation:/,$p'

# updateMode 仍是 Off，api 的 Pod 没有被重建。
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.containers[0].resources}{"\n"}'
kubectl -n shorty get pods -l app.kubernetes.io/name=api
