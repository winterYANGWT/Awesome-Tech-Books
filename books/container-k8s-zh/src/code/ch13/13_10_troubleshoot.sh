# 13.10：验签失败时先确认三件事：验的是不是 digest、策略匹配什么、webhook 是不是 Fail
REG=$(cat /tmp/ch13-registry)
cd /tmp/ch13-keys

#> expect: sha256-.*\.sig
cosign tree "$REG/shorty:0.1"

#> expect: Fail
kubectl get validatingwebhookconfiguration policy.sigstore.dev \
  -o jsonpath='{.webhooks[0].failurePolicy}'; echo

#> expect: shorty\*\*
kubectl get clusterimagepolicy shorty-signed \
  -o jsonpath='{.spec.images[0].glob}'; echo

#> expect: 1/1
kubectl -n cosign-system get deploy policy-controller-webhook
