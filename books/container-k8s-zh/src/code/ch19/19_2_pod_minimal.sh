kubectl -n shorty apply -f pod_minimal.yaml
kubectl -n shorty wait --for=condition=Ready pod/api-pod --timeout=120s
kubectl -n shorty get pod api-pod -o wide

# 直接创建的 Pod 没有控制器；删掉之后，没有任何东西会把它补回来。
kubectl -n shorty delete pod api-pod

#> expect: No resources found
kubectl -n shorty get pods -l app.kubernetes.io/name=api-pod
