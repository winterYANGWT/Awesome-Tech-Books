# 34.3 精简 Argo CD：不装 UI、Dex 和通知，用 kubectl 看 Application。
helm repo add argo https://argoproj.github.io/argo-helm >/dev/null
#> timeout: 600
helm upgrade --install argocd argo/argo-cd --version 10.11.6 \
    -n argocd --create-namespace -f argocd-values.yaml --wait --timeout 480s

kubectl -n argocd get deploy,statefulset,pod

# 本地 Gitea（SQLite），只给 Argo CD 当 Git 源。
kubectl apply -f gitea.yaml
kubectl -n git rollout status deploy/gitea --timeout=180s
kubectl -n git get pods

# 建管理员账号，后面用它通过 API 建仓库、用 HTTP push。
kubectl -n git exec deploy/gitea -- su-exec git gitea admin user create \
    --admin --username git --password shorty-git \
    --email git@shorty.local --must-change-password=false

# 把本章的 Kustomize 目录复制成一个独立 Git 仓库。
REPO=/book/build/scratch/ch34/repo
rm -rf "$REPO"
mkdir -p "$REPO"
cp -r kustomize/base kustomize/overlays "$REPO/"
git -C "$REPO" init -b main >/dev/null
git -C "$REPO" add -A
git -C "$REPO" -c user.name=book -c user.email=book@example.com \
    commit -q -m "shorty: base + dev/prod overlay"

# 从 lab 主机通过 port-forward 访问 Gitea 的 HTTP API 和 Git。
kubectl -n git port-forward svc/gitea 3000:3000 >/tmp/ch34-gitea-pf.log 2>&1 &
GITEA_PF=$!
curl -fsS --retry 60 --retry-delay 1 --retry-all-errors \
    http://127.0.0.1:3000/api/healthz >/dev/null 2>&1

curl -fsS -u git:shorty-git -H 'Content-Type: application/json' \
    -d '{"name":"shorty-deploy","private":false,"auto_init":false}' \
    http://127.0.0.1:3000/api/v1/user/repos | jq -r '.full_name'

git -C "$REPO" push -q \
    http://git:shorty-git@127.0.0.1:3000/git/shorty-deploy.git main
kill "$GITEA_PF" 2>/dev/null || true

kubectl apply -f argocd-project.yaml
kubectl apply -f application.yaml

#> timeout: 300
kubectl -n argocd wait --for=jsonpath='{.status.sync.status}'=Synced application/shorty --timeout=240s
#> timeout: 240
kubectl -n shorty rollout status deploy/api --timeout=180s
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
#> timeout: 180
kubectl -n argocd wait --for=jsonpath='{.status.health.status}'=Healthy application/shorty --timeout=120s

kubectl -n argocd get application shorty \
    -o custom-columns=NAME:.metadata.name,SYNC:.status.sync.status,HEALTH:.status.health.status
kubectl -n shorty get deploy api \
    -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
