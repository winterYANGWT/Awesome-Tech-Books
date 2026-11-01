# 用文件和字面量各建一个 ConfigMap。
kubectl -n shorty create configmap shorty-files \
  --from-literal=SHORTY_LOG_LEVEL=debug \
  --from-file=app.properties=app.properties
kubectl -n shorty get configmap shorty-files -o jsonpath='{.data}'

# data 存 UTF-8，binaryData 存 base64 编码的二进制。
kubectl apply -f binary-configmap.yaml
kubectl -n shorty get configmap shorty-binary -o jsonpath='{.data}{"\n"}{.binaryData}'

# api 的 Deployment 把整个 shorty-config 作为环境变量来源。
kubectl -n shorty get configmap shorty-config -o jsonpath='{.data}'
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.containers[0].envFrom[0]}'

# 改一个非敏感配置，重启后从启动日志看它是否生效。
kubectl -n shorty patch configmap shorty-config --type=merge \
  -p '{"data":{"SHORTY_BASE_URL":"http://shorty.local"}}'
kubectl -n shorty rollout restart deployment/api
kubectl -n shorty rollout status deployment/api --timeout=180s
POD=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api --sort-by=.metadata.creationTimestamp -o name | tail -n 1)
kubectl -n shorty logs "$POD" --tail=-1 | grep 'shorty starting'
