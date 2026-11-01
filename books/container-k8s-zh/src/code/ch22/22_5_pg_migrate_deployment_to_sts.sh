# 22.5 把 shorty 的 postgres 从 Deployment+PVC 迁到 StatefulSet。
# 第一步：停掉写入方（api 和 worker），迁移期间不再有新数据进旧库。
kubectl -n shorty scale deployment/api --replicas=0
kubectl -n shorty scale deployment/worker --replicas=0
kubectl -n shorty rollout status deployment/api --timeout=120s
kubectl -n shorty rollout status deployment/worker --timeout=120s

# 准备固定的测试数据，并记下迁移前的行数。
kubectl apply -f job-pg-seed.yaml
kubectl -n shorty wait --for=condition=complete job/pg-seed --timeout=120s
kubectl -n shorty logs job/pg-seed
kubectl -n shorty delete job pg-seed --wait=true

# 建备份卷，然后用 Job 通过 Service 连旧库做 pg_dump。
kubectl apply -f pg-backup-pvc.yaml
kubectl apply -f job-pg-dump.yaml
kubectl -n shorty wait --for=condition=complete job/pg-dump --timeout=180s
kubectl -n shorty logs job/pg-dump
kubectl -n shorty delete job pg-dump --wait=true

# 旧的 Deployment 可以删了，旧 PVC postgres-data 保留，作为回滚点。
kubectl -n shorty delete deployment postgres --wait=true
kubectl -n shorty wait --for=delete pod -l app.kubernetes.io/name=postgres --timeout=120s
kubectl -n shorty get pvc postgres-data

# ClusterIP 和 headless 不能并存于同一个 Service，先删再建。
kubectl -n shorty delete service postgres --wait=true
kubectl apply -f postgres-headless-service.yaml

# 建 StatefulSet。volumeClaimTemplates 生成 data-postgres-0。
kubectl apply -f postgres-sts.yaml
kubectl -n shorty rollout status statefulset/postgres --timeout=180s
kubectl -n shorty get statefulset postgres
kubectl -n shorty get pod,pvc

# 把备份恢复到新库，并核对行数。
kubectl apply -f job-pg-restore.yaml
kubectl -n shorty wait --for=condition=complete job/pg-restore --timeout=180s
kubectl -n shorty logs job/pg-restore
kubectl -n shorty delete job pg-restore --wait=true

# 恢复写入方，等 api 就绪。
kubectl -n shorty scale deployment/api --replicas=2
kubectl -n shorty scale deployment/worker --replicas=1
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty rollout status deployment/worker --timeout=180s

# 从集群内部访问 /readyz，确认 api 能连上新库。
kubectl apply -f job-readyz.yaml
kubectl -n shorty wait --for=condition=complete job/readyz-check --timeout=120s
kubectl -n shorty logs job/readyz-check
kubectl -n shorty delete job readyz-check --wait=true

# 最终状态：StatefulSet、headless Service 的 Endpoints、新旧 PVC 都在。
kubectl -n shorty get statefulset,pod,service,pvc
kubectl -n shorty get endpointslice -l kubernetes.io/service-name=postgres -o custom-columns=NAME:.metadata.name,ADDRESSES:.endpoints[*].addresses,PORTS:.ports[*].port
