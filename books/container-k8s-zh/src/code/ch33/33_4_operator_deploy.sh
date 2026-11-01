# 把单副本换成 2 副本：先缩到 0，删掉旧 Lease，模拟一次干净的滚动启动。
kubectl -n shorty scale deployment/shorty-operator --replicas=0

kubectl -n shorty rollout status deployment/shorty-operator --timeout=120s

kubectl -n shorty delete lease shorty-operator-lock --ignore-not-found

kubectl -n shorty scale deployment/shorty-operator --replicas=2

kubectl -n shorty rollout status deployment/shorty-operator --timeout=180s

kubectl -n shorty get pods -l app.kubernetes.io/name=shorty-operator --field-selector=status.phase=Running

kubectl -n shorty wait --for=create lease/shorty-operator-lock --timeout=60s

kubectl -n shorty get lease shorty-operator-lock -o jsonpath='{.spec.holderIdentity}{"\n"}'

kubectl -n shorty apply -f shorty-backup-2.yaml

kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Completed shortybackup/shorty-backup-2 --timeout=180s

kubectl -n shorty get job shorty-backup-2-pgdump

#> head: 12
kubectl -n shorty logs -l app.kubernetes.io/name=shorty-operator --prefix --tail=200 | grep -E 'started|became leader|standby' | sort

kubectl -n shorty logs -l app.kubernetes.io/name=shorty-operator --prefix --tail=200 | grep 'shorty-backup-2' | tail -6
