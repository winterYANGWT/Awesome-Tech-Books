# 1) 控制器不在：CR 能建，但没人加 finalizer，也没有 Job 和 status。
kubectl -n shorty scale deployment/shorty-operator --replicas=0

kubectl -n shorty rollout status deployment/shorty-operator --timeout=120s

kubectl -n shorty apply -f shorty-backup-late.yaml

kubectl -n shorty get shortybackup.v1.shorty.example.com shorty-backup-late -o jsonpath='finalizers={.metadata.finalizers} phase={.status.phase}{"\n"}'

#> fails
#> expect: NotFound
kubectl -n shorty get job shorty-backup-late-pgdump

# 2) 删一个带 finalizer 的 CR：控制器不在，它会停在 Terminating。
kubectl -n shorty delete shortybackup.v1.shorty.example.com shorty-backup-2 --wait=false

kubectl -n shorty get shortybackup.v1.shorty.example.com shorty-backup-2 -o jsonpath='deletionTimestamp={.metadata.deletionTimestamp}{"\n"}finalizers={.metadata.finalizers[*]}{"\n"}'

kubectl -n shorty get job shorty-backup-2-pgdump

# 3) 控制器回来：旧 Lease 过期后新 leader 接管，补做 Job、清掉卡住的 CR。
kubectl -n shorty scale deployment/shorty-operator --replicas=2

kubectl -n shorty rollout status deployment/shorty-operator --timeout=180s

kubectl -n shorty wait --for=delete shortybackup.v1.shorty.example.com/shorty-backup-2 --timeout=120s

kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Completed shortybackup.v1.shorty.example.com/shorty-backup-late --timeout=120s

kubectl -n shorty get job shorty-backup-late-pgdump

# 4) 把 Role 收窄到不能 create Job：CR 的 status 会写 Failed/JobCreateFailed，日志里有 Forbidden。
kubectl apply -f operator-rbac-readonly.yaml

kubectl -n shorty apply -f shorty-backup-denied.yaml

kubectl -n shorty wait --for=jsonpath='{.status.conditions[?(@.type=="Ready")].reason}'=JobCreateFailed shortybackup.v1.shorty.example.com/shorty-backup-denied --timeout=60s

kubectl -n shorty get shortybackup.v1.shorty.example.com shorty-backup-denied -o jsonpath='phase={.status.phase} reason={.status.conditions[?(@.type=="Ready")].reason}{"\n"}'

#> hide
kubectl -n shorty logs -l app.kubernetes.io/name=shorty-operator --prefix --tail=100 > /tmp/ch33-operator.log

grep -m1 'create job shorty-backup-denied-pgdump failed' /tmp/ch33-operator.log

# 5) 恢复 RBAC，控制器下一次 reconcile 就会把 Job 建出来。
kubectl apply -f operator-rbac.yaml

kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Completed shortybackup.v1.shorty.example.com/shorty-backup-denied --timeout=120s

kubectl -n shorty get job shorty-backup-denied-pgdump

# 清掉排障用的 CR，留下 shorty-backup。
kubectl -n shorty delete shortybackup.v1.shorty.example.com shorty-backup-late shorty-backup-denied --wait=false

#> hide
for i in $(seq 1 120); do
  [ "$(kubectl -n shorty get shortybackups.v1.shorty.example.com --no-headers 2>/dev/null | wc -l)" -eq 1 ] && break
  sleep 1
done

kubectl -n shorty get shortybackups.v1.shorty.example.com
