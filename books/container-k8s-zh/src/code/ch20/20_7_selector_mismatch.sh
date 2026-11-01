#> fails
#> expect: selector.*does not match template.*labels
kubectl -n shorty apply -f api-bad-selector.yaml
kubectl -n shorty apply -f api-fixed-selector.yaml
kubectl -n shorty rollout status deployment/api-bad-selector --timeout=120s
kubectl -n shorty get deployment api-bad-selector
kubectl -n shorty delete deployment api-bad-selector
kubectl -n shorty get deployment
