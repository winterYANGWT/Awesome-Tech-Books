# 18.4 三种管理方式，以及两个 field manager 的冲突
#> expect: kubectl-client-side-apply
kubectl -n shorty get deploy api -o json --show-managed-fields | jq -r '.metadata.managedFields[] | "\(.manager)\t\(.operation)\t\(.apiVersion)"'

#> expect: deployment.apps/api serverside-applied
kubectl apply --server-side --field-manager=team-a --force-conflicts -f api-deployment.yaml

#> expect: team-a
kubectl -n shorty get deploy api -o json --show-managed-fields | jq -r '.metadata.managedFields[] | "\(.manager)\t\(.operation)\t\(.apiVersion)"'

# 第二个 manager 想把 replicas 改成 2，但它不是 replicas 的当前属主。
#> fails
#> expect: Apply failed with 1 conflict
#> expect: team-a
#> expect: \.spec\.replicas
kubectl apply --server-side --field-manager=team-b -f api-replicas-2.yaml

#> expect: ^1$
kubectl -n shorty get deploy api -o jsonpath='{.spec.replicas}{"\n"}'
