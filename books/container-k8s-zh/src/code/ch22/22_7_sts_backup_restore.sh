# 22.7 备份与恢复演练：先记录行数，再逻辑备份、删表、恢复、验证应用连通。
kubectl apply -f job-pg-count.yaml
kubectl -n shorty wait --for=condition=complete job/pg-count --timeout=120s
kubectl -n shorty logs job/pg-count
kubectl -n shorty delete job pg-count --wait=true

# pg_dump 写到独立的备份卷。
kubectl apply -f job-pg-backup.yaml
kubectl -n shorty wait --for=condition=complete job/pg-backup --timeout=180s
kubectl -n shorty logs job/pg-backup
kubectl -n shorty delete job pg-backup --wait=true
kubectl -n shorty get pvc postgres-backup

# 模拟误删表。
kubectl apply -f job-pg-drop.yaml
kubectl -n shorty wait --for=condition=complete job/pg-drop --timeout=120s
kubectl -n shorty logs job/pg-drop
kubectl -n shorty delete job pg-drop --wait=true

# 从备份卷恢复，记录 RTO，并核对行数。
kubectl apply -f job-pg-backup-restore.yaml
kubectl -n shorty wait --for=condition=complete job/pg-backup-restore --timeout=180s
kubectl -n shorty logs job/pg-backup-restore
kubectl -n shorty delete job pg-backup-restore --wait=true

# 数据回来了还不够，应用要能连上。
kubectl apply -f job-readyz.yaml
kubectl -n shorty wait --for=condition=complete job/readyz-check --timeout=120s
kubectl -n shorty logs job/readyz-check
kubectl -n shorty delete job readyz-check --wait=true
