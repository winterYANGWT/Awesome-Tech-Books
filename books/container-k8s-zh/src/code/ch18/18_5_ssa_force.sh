# 18.5 强制接管、dry-run 和 managedFields
#> expect: team-a
kubectl -n shorty get deploy api -o json --show-managed-fields | jq -r '.metadata.managedFields[] | select(.fieldsV1."f:spec"."f:replicas") | "replicas owned by \(.manager) (\(.operation))"'

#> expect: deployment.apps/api serverside-applied
kubectl apply --server-side --field-manager=team-b --force-conflicts -f api-replicas-2.yaml

#> expect: team-b
kubectl -n shorty get deploy api -o json --show-managed-fields | jq -r '.metadata.managedFields[] | select(.fieldsV1."f:spec"."f:replicas") | "replicas owned by \(.manager) (\(.operation))"'

# 只看不写：diff 以 server-side dry-run 算出 replicas 会从 2 变成 1，命令以 1 退出。
#> fails
#> expect: replicas: 2
#> expect: replicas: 1
kubectl diff --server-side --field-manager=team-b --force-conflicts -f api-replicas-1.yaml

#> expect: server dry run
kubectl apply --server-side --field-manager=team-b --dry-run=server -f api-replicas-1.yaml

#> expect: ^2$
kubectl -n shorty get deploy api -o jsonpath='{.spec.replicas}{"\n"}'

#> expect: serverside-applied
kubectl apply --server-side --field-manager=team-b -f api-replicas-1.yaml

#> expect: successfully rolled out
kubectl -n shorty rollout status deployment/api --timeout=120s

#> expect: ^1 1$
kubectl -n shorty get deploy api -o jsonpath='{.spec.replicas}{" "}{.status.availableReplicas}{"\n"}'

#> expect: serverside-applied
kubectl apply --server-side --field-manager=team-a --force-conflicts -f api-service.yaml

#> expect: team-a Apply
kubectl -n shorty get svc api -o json --show-managed-fields | jq -r '.metadata.managedFields[] | "\(.manager) \(.operation)"'
