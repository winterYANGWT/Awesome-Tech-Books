kubectl apply -f qos-pods.yaml
kubectl -n ch28-qos wait --for=condition=Ready pod/guaranteed pod/burstable pod/besteffort --timeout=120s
# 按名字排序，保证输出顺序稳定。
kubectl -n ch28-qos get pod -o jsonpath='{range .items[*]}{.metadata.name}{" "}{.status.qosClass}{"\n"}{end}' | sort
kubectl -n ch28-qos get pod guaranteed -o jsonpath='{.spec.containers[0].resources}{"\n"}'
