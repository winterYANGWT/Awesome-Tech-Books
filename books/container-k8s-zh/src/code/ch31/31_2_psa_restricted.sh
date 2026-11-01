# 31.2 先在服务端 dry-run：只评估现有 Pod，不真的打标签。
# 输出里的 Warning 会逐个列出 shorty 里过不了 restricted 的 Pod。
kubectl label --dry-run=server --overwrite namespace shorty pod-security.kubernetes.io/enforce=restricted

# 真正打上三档标签：enforce 拒绝、warn 返回警告、audit 写审计注解。
# enforce-version 把检查锁在 v1.37，升级集群时策略不会跟着变。
kubectl label namespace shorty \
  pod-security.kubernetes.io/enforce=restricted \
  pod-security.kubernetes.io/enforce-version=v1.37 \
  pod-security.kubernetes.io/warn=restricted \
  pod-security.kubernetes.io/audit=restricted

# 直接创建 Pod：被拒绝的是 Pod 本身，错误里会列出所有违反的字段。
#> fails
#> expect: violates PodSecurity "restricted:v1.37"
#> expect: allowPrivilegeEscalation != false
#> expect: seccompProfile
kubectl apply -f psa-root-pod.yaml

# 换成 Deployment：apply 成功，Pod 由 ReplicaSet 去创建，拒绝出现在 Event 里。
kubectl apply -f psa-root-deploy.yaml

# 等 ReplicaSet 把失败 Event 写出来，再读那条 Event 的 message。
for i in $(seq 1 30); do
  msg=$(kubectl -n shorty get events --field-selector reason=FailedCreate \
    -o jsonpath='{range .items[*]}{.involvedObject.name}{": "}{.message}{"\n"}{end}' \
    | grep psa-root-demo || true)
  [ -n "$msg" ] && break
  sleep 1
done
printf '%s\n' "$msg" | head -1

# 这个 Deployment 不会创建出 Pod，删掉它，避免 ReplicaSet 一直重试。
kubectl -n shorty delete deployment psa-root-demo
