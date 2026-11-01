# 先看 CRD 本身：apply 之后等 Established，再用 API 发现和 explain 看它。
kubectl apply -f crd-v1alpha1.yaml

# status.conditions 可能还没写出来，kubectl wait 会直接报 accessor error；先循环等 Established。
#> hide
for i in $(seq 1 60); do
  kubectl get crd shortybackups.shorty.example.com -o jsonpath='{range .status.conditions[*]}{.type}{"\n"}{end}' 2>/dev/null | grep -qx Established && break
  sleep 1
done

# OpenAPI 的发布比 Established 晚一点，等 discovery 里出现这个组版本再 explain。
#> hide
for i in $(seq 1 30); do
  kubectl get --raw /openapi/v3 2>/dev/null | grep -q 'apis/shorty.example.com/v1alpha1' && break
  sleep 1
done

kubectl get crd shortybackups.shorty.example.com

kubectl api-resources | grep shortybackups

#> head: 25
kubectl explain shortybackup.spec
