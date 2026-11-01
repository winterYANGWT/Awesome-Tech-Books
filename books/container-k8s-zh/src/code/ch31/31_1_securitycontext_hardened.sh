# 31.1 先确认 api 的 Pod 模板里没有任何 securityContext。
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.securityContext}{"\n"}{.spec.template.spec.containers[0].securityContext}{"\n"}'

# 记下旧 Pod，滚动更新后等它们真正消失，输出里就只剩新模板创建的 Pod。
#> hide
old_pods=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}')

# 打上 Pod 级和容器级的加固项，等滚动更新完成。
kubectl -n shorty patch deployment api --type=strategic --patch-file api-securitycontext-patch.yaml
kubectl -n shorty rollout status deployment/api --timeout=120s

#> hide
for pod in $old_pods; do kubectl -n shorty wait --for=delete "pod/$pod" --timeout=60s >/dev/null; done

# 看新 Pod 上实际生效的字段：Pod 级的 runAsNonRoot/fsGroup 和容器级的只读根、drop ALL、seccomp。
kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{range .items[*]}{.metadata.name}{"\n  pod: "}{.spec.securityContext}{"\n  container: "}{.spec.containers[0].securityContext}{"\n"}{end}'

# 再起一个带 shell 的 Pod，用同样的加固项观察运行时效果。
kubectl apply -f securitycontext-shell.yaml
kubectl -n shorty wait --for=condition=Ready pod/securitycontext-shell --timeout=60s

# 容器级 runAsUser 覆盖了 Pod 级的 1000。
kubectl -n shorty exec securitycontext-shell -- id

# emptyDir 卷仍然可写，组是 fsGroup 指定的 65532。
kubectl -n shorty exec securitycontext-shell -- stat -c 'uid=%u gid=%g mode=%a' /data
kubectl -n shorty exec securitycontext-shell -- sh -c 'echo ok > /data/probe && cat /data/probe'

# 根文件系统是只读的，往 / 写会失败。
#> fails
#> expect: Read-only file system
kubectl -n shorty exec securitycontext-shell -- touch /blocked

# 加固后的 api 仍然能通过 Service 回答就绪检查。
kubectl -n shorty exec securitycontext-shell -- wget -qO- http://api:8080/readyz

# 这个 shell Pod 只用于观察，删掉，给后面的 PSA 留一个干净的命名空间。
kubectl -n shorty delete pod securitycontext-shell
