# 31.6 旧 PSP 清单现在连 API 都找不到，apply 在客户端解析 REST 映射时就失败。
#> fails
#> expect: no matches for kind "PodSecurityPolicy"
kubectl apply -f psp-old.yaml

# kubectl get psp 的说法更直接：服务端没有这个资源类型。
#> fails
#> expect: doesn't have a resource type "psp"
kubectl get psp

# 替代方案：给 namespace 打 PSA 标签。baseline 允许默认配置的 root Pod。
kubectl create namespace psp-demo
kubectl label namespace psp-demo pod-security.kubernetes.io/enforce=baseline
kubectl -n psp-demo run psp-root --image=busybox:1.37 --restart=Never -- sleep 3600
kubectl -n psp-demo wait --for=condition=Ready pod/psp-root --timeout=60s
kubectl -n psp-demo get pod psp-root

# 同一个 Pod 在 restricted 下被拒绝：没有 runAsNonRoot、allowPrivilegeEscalation、
# drop ALL 和 seccompProfile，正好是 PSP 里那几条 runAsUser/requiredDropCapabilities 的替代。
kubectl -n psp-demo delete pod psp-root
kubectl label --overwrite namespace psp-demo pod-security.kubernetes.io/enforce=restricted
#> fails
#> expect: violates PodSecurity "restricted:latest"
#> expect: runAsNonRoot != true
kubectl -n psp-demo run psp-root --image=busybox:1.37 --restart=Never -- sleep 3600

# 演示用的 namespace 删掉，避免留下一个只有标签的 namespace。
kubectl delete namespace psp-demo
