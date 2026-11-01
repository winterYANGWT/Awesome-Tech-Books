# 31.7 先看已经在 enforce=restricted 的 shorty：再 dry-run 同一个值，
# kubectl 发现标签没变，只回一句 not labeled，不会重新评估现有 Pod。
kubectl label --dry-run=server --overwrite namespace shorty pod-security.kubernetes.io/enforce=restricted

# 换一个还没打标签的 namespace，放一个默认配置的 root Pod，再 dry-run restricted。
# 这次会逐个列出过不了检查的 Pod，以及具体违反了哪几条。
kubectl create namespace triage-demo
kubectl -n triage-demo run legacy --image=busybox:1.37 --restart=Never -- sleep 3600
kubectl -n triage-demo wait --for=condition=Ready pod/legacy --timeout=60s
#> expect: legacy
#> expect: runAsNonRoot != true
kubectl label --dry-run=server --overwrite namespace triage-demo pod-security.kubernetes.io/enforce=restricted
kubectl delete namespace triage-demo

# CEL 类型错误：策略能创建，但 status.typeChecking 会指出字段不存在。
kubectl apply -f vap-typecheck-pod.yaml
for i in $(seq 1 30); do
  warnings=$(kubectl get validatingadmissionpolicy demo-typecheck-pod \
    -o jsonpath='{range .status.typeChecking.expressionWarnings[*]}{.fieldRef}{"\n"}{.warning}{"\n"}{end}' 2>/dev/null || true)
  [ -n "$warnings" ] && break
  sleep 1
done
printf '%s\n' "$warnings"

# CEL 语法错误：创建阶段就被拒绝，不会留下一个“坏掉但存在”的策略对象。
#> fails
#> expect: compilation failed
kubectl apply -f vap-syntax-error.yaml

# 清掉演示策略；它没有绑定，本来也不会拦截请求。
kubectl delete validatingadmissionpolicy demo-typecheck-pod
