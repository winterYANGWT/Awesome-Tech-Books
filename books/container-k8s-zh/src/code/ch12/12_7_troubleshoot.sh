# 12.7 排障：HTTP registry 与 ImagePullBackOff
# 第一个故障：用非 localhost 的地址推一个只讲 HTTP 的 registry。
addr=$(hostname -i | awk '{print $1}')
printf 'eth0 地址：%s\n' "$addr"

docker tag shorty:0.1 "$addr:5000/shorty:tls"

#> fails
#> expect: http: server gave HTTP response to HTTPS client
docker push "$addr:5000/shorty:tls" 2>&1 | tail -2

# 修复：本地实验把 registry 放回 localhost，Docker 对 127.0.0.0/8 默认走 HTTP。
printf '%s' s3cret | docker login localhost:5000 -u shorty --password-stdin
docker tag shorty:0.1 localhost:5000/shorty:tls
docker push localhost:5000/shorty:tls 2>&1 | tail -2

# 第二个故障：Secret 存在，但 docker-server 写的是另一个端口，凭据匹配不上。
kubectl -n shorty create secret docker-registry wrongreg --docker-server=localhost:5001 --docker-username=shorty --docker-password=s3cret

#> expect: ^\["localhost:5001"\]$
kubectl -n shorty get secret wrongreg -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq -c '.auths | keys'

kubectl -n shorty apply -f pod-wrong-secret.yaml

kubectl -n shorty wait --for=jsonpath='{.status.containerStatuses[0].state.waiting.reason}'=ImagePullBackOff pod/wrong-secret --timeout=120s

#> expect: no basic auth credentials
kubectl -n shorty get events -o json | jq -r '.items[] | select(.involvedObject.name == "wrong-secret" and .reason == "Failed") | select(.message | contains("Failed to pull image")) | .message' | tail -1

kubectl -n shorty delete pod wrong-secret --wait=true
kubectl -n shorty delete secret wrongreg

# 实验结束，把 12.5 用的单节点集群删掉，释放 lab 内存。
#> hide
kind delete cluster --name shorty >/dev/null 2>&1 || true
