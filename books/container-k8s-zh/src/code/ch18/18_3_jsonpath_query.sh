# 18.3 kubectl 的输出、发现和 kubeconfig
kubectl -n shorty get pods -o wide

#> expect: 10\.244\.
kubectl -n shorty get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.podIP}{"\t"}{.spec.nodeName}{"\t"}{.status.containerStatuses[0].restartCount}{"\n"}{end}'

#> expect: NAME\s+IP\s+NODE\s+RESTARTS
kubectl -n shorty get pods -o custom-columns='NAME:.metadata.name,IP:.status.podIP,NODE:.spec.nodeName,RESTARTS:.status.containerStatuses[0].restartCount'

kubectl get pods -A --sort-by=.metadata.name -o custom-columns='NAMESPACE:.metadata.namespace,NAME:.metadata.name' | sed -n '1,8p'

#> expect: apps/v1
kubectl api-versions | grep '^apps/'

kubectl api-resources --namespaced=true --verbs=list -o name | grep -E '^(deployments|pods|services)'

#> expect: containers\s+<\[\]Container>
#> expect: image\s+<string>
kubectl explain pod.spec.containers --recursive --max-depth=1

#> expect: Normal
kubectl -n shorty get events --sort-by=.metadata.creationTimestamp -o custom-columns='TYPE:.type,REASON:.reason,OBJECT:.involvedObject.name' | tail -8

#> expect: kind-shorty
kubectl config current-context

kubectl config set-context --current --namespace=shorty

#> expect: shorty
kubectl config view --minify -o jsonpath='{.contexts[0].context.namespace}{"\n"}'

kubectl config set-context --current --namespace=default
