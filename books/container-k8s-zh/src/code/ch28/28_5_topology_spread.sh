kubectl apply -f spread-demo.yaml
kubectl -n shorty rollout status deployment/spread-demo --timeout=180s
kubectl -n shorty get pod -l app=spread-demo -o wide
# 只统计每个节点上的副本数，便于确认 2/2。
kubectl -n shorty get pod -l app=spread-demo -o custom-columns=NODE:.spec.nodeName --no-headers | sort | uniq -c
#> hide
kubectl -n shorty delete deployment spread-demo --wait=true
