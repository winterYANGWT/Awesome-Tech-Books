# 控制器先跑起来：RBAC、Deployment，然后创建一个 CR 看它变出 Job。
kubectl apply -f operator-rbac.yaml

kubectl apply -f operator-networkpolicy.yaml

kubectl apply -f operator-deployment.yaml

kubectl -n shorty rollout status deployment/shorty-operator --timeout=180s

kubectl -n shorty apply -f shorty-backup.yaml

# 等 CR 的 status.phase 变成 Completed；这比等 Job 稳，Job 还没建时也能等。
kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Completed shortybackup/shorty-backup --timeout=180s

kubectl -n shorty get job shorty-backup-pgdump

kubectl -n shorty get pvc shorty-backup-data

kubectl -n shorty logs job/shorty-backup-pgdump --tail=5
