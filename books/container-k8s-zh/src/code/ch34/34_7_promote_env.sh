# 34.7 同一 artifact 提升：只构建一次 0.2，先给 dev，再把同一个 digest 给 prod。
docker build -q --build-arg VERSION=0.2 -t localhost:5000/shorty:0.2 ../app/api
docker push -q localhost:5000/shorty:0.2
digest=$(docker image inspect --format '{{range .RepoDigests}}{{println .}}{{end}}' \
    localhost:5000/shorty:0.2 | awk -F@ 'NF == 2 {print $2; exit}')
printf 'digest=%s\n' "$digest"

REPO=/book/build/scratch/ch34/repo
sed -i "s|newTag: \"0.1\"|digest: $digest|" "$REPO/overlays/dev/kustomization.yaml"
git -C "$REPO" add -A
git -C "$REPO" -c user.name=book -c user.email=book@example.com \
    commit -q -m "dev: shorty 0.2"

# 用 dev 里刚写入的那一行提升到 prod；不重新构建，也不改镜像 tag。
grep 'digest:' "$REPO/overlays/dev/kustomization.yaml"
sed -i "s|newTag: \"0.1\"|digest: $digest|" "$REPO/overlays/prod/kustomization.yaml"
git -C "$REPO" add -A
git -C "$REPO" -c user.name=book -c user.email=book@example.com \
    commit -q -m "prod: promote shorty 0.2"
git -C "$REPO" log --oneline -2

kubectl kustomize "$REPO/overlays/prod" \
    | yq eval-all -r 'select(.kind == "Deployment") | .spec.template.spec.containers[0].image'

kubectl -n git port-forward svc/gitea 3000:3000 >/tmp/ch34-gitea-pf.log 2>&1 &
GITEA_PF=$!
curl -fsS --retry 60 --retry-delay 1 --retry-all-errors \
    http://127.0.0.1:3000/api/healthz >/dev/null 2>&1
git -C "$REPO" push -q \
    http://git:shorty-git@127.0.0.1:3000/git/shorty-deploy.git main
kill "$GITEA_PF" 2>/dev/null || true

kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n shorty wait --for=jsonpath='{.spec.template.spec.containers[0].image}'="localhost:5000/shorty@$digest" deployment/api --timeout=240s
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n argocd wait --for=jsonpath='{.status.sync.status}'=Synced application/shorty --timeout=180s
kubectl -n argocd annotate application shorty argocd.argoproj.io/refresh=normal --overwrite >/dev/null
kubectl -n argocd wait --for=jsonpath='{.status.health.status}'=Healthy application/shorty --timeout=180s

kubectl -n shorty get deployment api \
    -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
kubectl -n argocd get application shorty \
    -o custom-columns=NAME:.metadata.name,SYNC:.status.sync.status,HEALTH:.status.health.status
