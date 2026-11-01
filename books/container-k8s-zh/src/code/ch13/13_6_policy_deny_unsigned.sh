# 13.6：在准入层验签。集群和 policy-controller 的安装用 hide 跳过，只显示策略效果
REG=$(cat /tmp/ch13-registry)
DIGEST=$(cat /tmp/ch13-digest)
TLS=/book/build/scratch/ch13/tls

#> hide
source ../lib/common.sh
#> hide
#> timeout: 600
cluster_up kind-1n.yaml
#> hide
NODE=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
#> hide
docker cp "$TLS/ca.crt" "$NODE:/usr/local/share/ca-certificates/shorty-registry.crt"
#> hide
docker exec "$NODE" update-ca-certificates >/dev/null 2>&1
#> hide
docker exec "$NODE" systemctl restart containerd
#> hide
kubectl wait --for=condition=Ready "node/$NODE" --timeout=180s >/dev/null

# 还没装 policy-controller：kubelet 会照常拉未签名镜像，说明运行时并不替我们验签。
kubectl create namespace shorty
kubectl -n shorty run unsigned-before --image="$REG/shorty:unsigned" --restart=Never
kubectl -n shorty wait --for=condition=Ready pod/unsigned-before --timeout=120s
kubectl -n shorty delete pod unsigned-before >/dev/null

#> hide
kubectl create namespace cosign-system >/dev/null
#> hide
kubectl -n cosign-system create configmap registry-ca --from-file=ca.crt="$TLS/ca.crt" >/dev/null
#> hide
helm upgrade --install policy-controller ./policy-controller-0.10.8.tgz -n cosign-system \
  --set webhook.registryCaBundle.name=registry-ca \
  --set webhook.registryCaBundle.key=ca.crt \
  --set webhook.env.SSL_CERT_FILE=/etc/custom-ca/ca.crt \
  --set webhook.extraArgs.disable-tuf=true \
  --set webhook.replicaCount=1 >/dev/null
#> hide
kubectl -n cosign-system rollout status deploy/policy-controller-webhook --timeout=300s >/dev/null
#> hide
for _ in $(seq 1 60); do
  [ -n "$(kubectl -n cosign-system get endpoints webhook -o jsonpath='{.subsets[*].addresses[*].ip}' 2>/dev/null)" ] && break
  sleep 1
done
#> hide
kubectl label namespace shorty policy.sigstore.dev/include=true >/dev/null
#> hide
kubectl -n cosign-system create secret generic cosign-pub \
  --from-file=cosign.pub=/tmp/ch13-keys/cosign.pub >/dev/null

# webhook 的 Service 端点就绪和 kube-proxy 规则生效之间可能差几秒，apply 失败就重试。
applied=
for _ in $(seq 1 30); do
  if out=$(sed "s|__REGISTRY__|$REG|" clusterimagepolicy.yaml | kubectl apply -f - 2>&1); then
    printf '%s\n' "$out"
    applied=1
    break
  fi
  sleep 2
done
[ -n "$applied" ]

# audit：warn 模式放行未签名镜像，但返回 Warning。
#> expect: signature key validation failed
#> expect: no signatures found
kubectl -n shorty run unsigned-warn --image="$REG/shorty:unsigned" --restart=Never
kubectl -n shorty delete pod unsigned-warn --ignore-not-found >/dev/null

kubectl patch clusterimagepolicy shorty-signed --type=merge -p '{"spec":{"mode":"enforce"}}'

#> fails
#> expect: no signatures found
kubectl -n shorty run unsigned --image="$REG/shorty:unsigned" --restart=Never

# 对象没有创建，所以不会有 Pod 的 Event，也没有 Pod 可看。
kubectl -n shorty get pods

kubectl -n shorty run signed --image="$REG/shorty@$DIGEST" --restart=Never
kubectl -n shorty wait --for=condition=Ready pod/signed --timeout=180s
kubectl -n shorty get pod signed
