# 12.5 Kubernetes 拉私有镜像
# 先不放 Secret，让 Pod 自己报错；失败信息里能同时看到 Secret 缺失和认证被拒。
kubectl -n shorty get serviceaccount default --no-headers -o custom-columns=NAME:.metadata.name

kubectl -n shorty apply -f pod-private.yaml

kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].state.waiting.reason}'=ImagePullBackOff pod/private-pull --timeout=120s

kubectl -n shorty get pod private-pull -o jsonpath='{.status.containerStatuses[0].state.waiting.reason}{"\n"}'

#> expect: Unable to retrieve some image pull secrets
kubectl -n shorty get events -o json | jq -r '.items[] | select(.involvedObject.name == "private-pull" and .reason == "FailedToRetrieveImagePullSecret") | .message' | tail -1

#> expect: no basic auth credentials
kubectl -n shorty get events -o json | jq -r '.items[] | select(.involvedObject.name == "private-pull" and .reason == "Failed") | select(.message | contains("Failed to pull image")) | .message' | tail -1

# 补上 Secret，再建一次同样的 Pod。
kubectl -n shorty create secret docker-registry regcred --docker-server=localhost:5000 --docker-username=shorty --docker-password=s3cret

kubectl -n shorty get secret regcred -o jsonpath='{.type}{"\n"}'

kubectl -n shorty delete pod private-pull --wait=true

kubectl -n shorty apply -f pod-private.yaml

kubectl -n shorty wait --for=jsonpath='{.status.phase}'=Succeeded pod/private-pull --timeout=120s

#> expect: ^shorty dev$
kubectl -n shorty logs private-pull
