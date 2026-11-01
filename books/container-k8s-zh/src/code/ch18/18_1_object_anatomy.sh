# 18.1 拆开一个 Deployment：GVK、metadata、spec 和 status
#> expect: apps/v1
#> expect: Deployment
kubectl -n shorty get deploy api -o yaml | yq '.apiVersion, .kind, .metadata.name, .metadata.namespace'

kubectl -n shorty get deploy api -o yaml | yq '.metadata | {"labels": .labels, "annotation keys": (.annotations | keys), "finalizers": .finalizers, "ownerReferences": .ownerReferences}'

kubectl -n shorty get deploy api -o yaml | yq '{"spec keys": (.spec | keys), "status keys": (.status | keys)}'

#> expect: spec.replicas: 1
#> expect: status.availableReplicas: 1
kubectl -n shorty get deploy api -o yaml | yq '{"spec.replicas": .spec.replicas, "spec.strategy.type": .spec.strategy.type, "spec.strategy.rollingUpdate.maxSurge": .spec.strategy.rollingUpdate.maxSurge, "status.observedGeneration": .status.observedGeneration, "status.availableReplicas": .status.availableReplicas}'

#> expect: deployments\s+deploy\s+apps/v1\s+true\s+Deployment
kubectl api-resources --api-group=apps

#> expect: Deployment apps/v1 api
kubectl get --raw /apis/apps/v1/namespaces/shorty/deployments/api | jq -r '"\(.kind) \(.apiVersion) \(.metadata.name)"'

#> expect: RollingUpdate
#> expect: maxSurge
kubectl explain deployment.spec.strategy --recursive
