kubectl -n shorty apply -f pdb.yaml
kubectl -n shorty get poddisruptionbudget api
NODE=$(kubectl -n shorty get pod -l app.kubernetes.io/name=api -o jsonpath='{.items[0].spec.nodeName}')
echo "NODE=$NODE"
#> fails
#> expect: [Cc]annot evict pod.*disruption budget
#> timeout: 60
kubectl drain "$NODE" --ignore-daemonsets --delete-emptydir-data --timeout=10s
#> expect: SchedulingDisabled
kubectl get node "$NODE"
kubectl uncordon "$NODE"
kubectl -n shorty delete poddisruptionbudget api
kubectl -n shorty get poddisruptionbudget
