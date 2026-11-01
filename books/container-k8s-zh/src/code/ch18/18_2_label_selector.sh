# 18.2 名字空间、标签和注解
#> expect: namespace/shorty
kubectl get namespaces -o name

POD=$(kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{.items[0].metadata.name}')
#> expect: app.kubernetes.io/name=api
kubectl -n shorty get pod "$POD" --show-labels

#> expect: labeled
kubectl -n shorty label pod "$POD" tier=frontend env=dev

#> expect: frontend\s+dev
kubectl -n shorty get pods -l tier=frontend,env=dev -o custom-columns='NAME:.metadata.name,TIER:.metadata.labels.tier,ENV:.metadata.labels.env'

#> expect: env=dev
kubectl -n shorty get pods -l 'env in (dev,staging)' --show-labels

#> expect: labeled
kubectl -n shorty label pod "$POD" tier=backend --overwrite

#> expect: No resources found
kubectl -n shorty get pods -l tier=frontend

#> expect: pod/
kubectl -n shorty get pods -l tier=backend -o name

#> expect: annotated
kubectl -n shorty annotate pod "$POD" kubernetes.io/change-cause="tier changed to backend for label drill" --overwrite

#> expect: tier changed to backend for label drill
kubectl -n shorty get pod "$POD" -o jsonpath='{.metadata.annotations.kubernetes\.io/change-cause}{"\n"}'

#> expect: matchExpressions
kubectl explain deployment.spec.selector.matchExpressions
