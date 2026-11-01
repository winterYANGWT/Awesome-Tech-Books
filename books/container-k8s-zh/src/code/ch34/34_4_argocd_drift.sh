# 34.4 先关着 selfHeal：手工改副本数，Argo CD 只报 OutOfSync，不会自己改回来。
kubectl -n shorty scale deployment/api --replicas=5
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n argocd wait --for=jsonpath='{.status.sync.status}'=OutOfSync application/shorty --timeout=120s

kubectl -n argocd get application shorty \
    -o custom-columns=NAME:.metadata.name,SYNC:.status.sync.status,HEALTH:.status.health.status
kubectl -n shorty get deployment api \
    -o jsonpath='{.spec.replicas}{"\n"}'

# 打开 selfHeal，再刷新一次：Argo CD 会把集群里的 replicas 拉回 Git 里的 2。
kubectl -n argocd patch application shorty --type=merge \
    -p '{"spec":{"syncPolicy":{"automated":{"prune":false,"selfHeal":true}}}}'
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null

kubectl -n shorty wait --for=jsonpath='{.spec.replicas}'=2 deployment/api --timeout=120s
kubectl -n shorty wait --for=condition=Available deployment/api --timeout=120s
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n argocd wait --for=jsonpath='{.status.sync.status}'=Synced application/shorty --timeout=120s
kubectl -n argocd wait --for=jsonpath='{.status.health.status}'=Healthy application/shorty --timeout=120s

kubectl -n argocd get application shorty \
    -o custom-columns=NAME:.metadata.name,SYNC:.status.sync.status,HEALTH:.status.health.status
kubectl -n shorty get deployment api \
    -o jsonpath='{.spec.replicas}{"\n"}'
