# 16.4 把第 14 章的 Compose 模型转成 Kubernetes 清单。
rm -rf /tmp/ch16-bridge
docker compose -f ../app/compose.yaml bridge convert \
  -t docker/compose-bridge-kubernetes@sha256:8520c84287a5c051d6e8eba03fc51c1d7cdc08ac12d5dd2a0fb63078eb6272b4 \
  -o /tmp/ch16-bridge -y
find /tmp/ch16-bridge -maxdepth 2 -type f -name '*.yaml' -printf '%P\n' | sort

# 生成物里的 api Deployment 和 Service 能通过服务端校验。
kubectl apply -f namespace.yaml
kubectl apply --dry-run=server \
  -f /tmp/ch16-bridge/base/0-shorty-namespace.yaml \
  -f /tmp/ch16-bridge/base/api-deployment.yaml \
  -f /tmp/ch16-bridge/base/api-service.yaml

# 但一次性任务 migrate 被转成了 Deployment，Pod 模板里的 restartPolicy: Never 不合法。
#> fails
#> expect: restartPolicy.*Unsupported value.*Never
kubectl apply --dry-run=server -f /tmp/ch16-bridge/base/migrate-deployment.yaml

# 生成物与手写清单不是同一套约定：Service 名字、Deployment 策略和环境变量都不同。
for f in /tmp/ch16-bridge/base/api-expose.yaml api-service.yaml; do
  echo "--- $f"
  kubectl create --dry-run=client -o json -f "$f" \
    | jq '{name: .metadata.name, type: (.spec.type // "ClusterIP"), selector: .spec.selector, ports: [.spec.ports[] | {port, targetPort}]}'
done

for f in /tmp/ch16-bridge/base/api-deployment.yaml api-deployment.yaml; do
  echo "--- $f"
  kubectl create --dry-run=client -o json -f "$f" \
    | jq '{name: .metadata.name, replicas: .spec.replicas, strategy: (.spec.strategy.type // "RollingUpdate"), env: [.spec.template.spec.containers[0].env[]?.name], mountPaths: [.spec.template.spec.containers[0].volumeMounts[]?.mountPath]}'
done
