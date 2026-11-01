# finalizer 在控制器里加：先确认它在，再删 CR，观察 Job/PVC 被清理。
kubectl -n shorty apply -f shorty-backup-temp.yaml

kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Completed shortybackup/shorty-backup-temp --timeout=180s

kubectl -n shorty get shortybackup shorty-backup-temp -o jsonpath='{.metadata.finalizers[*]}{"\n"}'

# --wait=false 让 delete 立即返回；对象会停在 Terminating，直到 finalizer 被移除。
kubectl -n shorty delete shortybackup shorty-backup-temp --wait=false

kubectl -n shorty wait --for=delete shortybackup/shorty-backup-temp --timeout=60s

#> fails
#> expect: NotFound
kubectl -n shorty get job shorty-backup-temp-pgdump

#> fails
#> expect: NotFound
kubectl -n shorty get pvc shorty-backup-temp-data

kubectl -n shorty logs -l app.kubernetes.io/name=shorty-operator --prefix --tail=100 | grep 'cleanup shorty-backup-temp' | tail -2

kubectl -n shorty get shortybackups
