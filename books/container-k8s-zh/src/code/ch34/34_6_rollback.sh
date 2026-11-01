# 34.6 发布坏 digest：先关掉 Argo CD 的自动同步，单独看 Deployment 的回滚。
kubectl -n argocd patch application shorty --type=merge \
    -p '{"spec":{"syncPolicy":{"automated":{"enabled":false}}}}'

bad="localhost:5000/shorty@sha256:$(printf '%064d' 0)"
kubectl -n shorty annotate deployment/api kubernetes.io/change-cause="bad digest ${bad##*@}" --overwrite
kubectl -n shorty set image deployment/api api="$bad"

#> fails
#> timeout: 120
kubectl -n shorty rollout status deployment/api --timeout=30s
kubectl -n shorty get pods -l app.kubernetes.io/name=api -o wide
kubectl -n shorty rollout history deployment/api | tail -4

# 记录从 undo 到重新可用的时间；旧 Pod 一直在，所以这段时间不是停机时间。
undo_at=$(date +%s)
kubectl -n shorty rollout undo deployment/api
kubectl -n shorty rollout status deployment/api --timeout=180s
elapsed=$(( $(date +%s) - undo_at ))
if [ "$elapsed" -eq 0 ]; then
    echo 'rollback_seconds=<1'
else
    printf 'rollback_seconds=%d\n' "$elapsed"
fi

kubectl -n shorty get deployment api \
    -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'

# 把 GitOps 的自动同步打开，让 Application 回到 Git 里的状态。
kubectl -n argocd patch application shorty --type=merge \
    -p '{"spec":{"syncPolicy":{"automated":{"enabled":true,"prune":false,"selfHeal":true}}}}'
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n argocd wait --for=jsonpath='{.status.sync.status}'=Synced application/shorty --timeout=180s
kubectl -n argocd get application shorty \
    -o custom-columns=NAME:.metadata.name,SYNC:.status.sync.status,HEALTH:.status.health.status
