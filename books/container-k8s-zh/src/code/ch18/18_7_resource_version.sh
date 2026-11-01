# 18.7 复现 resourceVersion 缺失的更新错误
#> expect: customresourcedefinition.apiextensions.k8s.io/replacedemos.example.com created
kubectl apply -f replace-demo-crd.yaml

# --raw 直接发 PUT，跳过 kubectl replace 对 resourceVersion 的自动补全。
#> fails
#> expect: metadata.resourceVersion: Invalid value:
#> expect: must be specified for an update
kubectl replace --raw /apis/apiextensions.k8s.io/v1/customresourcedefinitions/replacedemos.example.com -f replace-demo-crd.json

# 正常做法：从活对象取出带 resourceVersion 的副本，再 replace。
kubectl get crd replacedemos.example.com -o yaml | yq 'del(.status, .metadata.managedFields)' > /tmp/replace-demo-crd.yaml

#> expect: customresourcedefinition.apiextensions.k8s.io/replacedemos.example.com replaced
kubectl replace -f /tmp/replace-demo-crd.yaml

#> expect: deleted
kubectl delete crd replacedemos.example.com
