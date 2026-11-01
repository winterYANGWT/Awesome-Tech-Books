# 用 Helm 装 NGINX Gateway Fabric 2.8.0；--wait 等控制器 Deployment 就绪。
helm upgrade --install ngf oci://ghcr.io/nginx/charts/nginx-gateway-fabric \
  --version 2.8.0 --namespace nginx-gateway --create-namespace \
  --wait --timeout 300s
#> tail: 20

# 控制器会注册自己的 GatewayClass；ACCEPTED 为 True 才说明它在处理这个类。
kubectl get gatewayclass
kubectl get gatewayclass nginx \
  -o jsonpath='{.spec.controllerName}{" accepted="}{.status.conditions[?(@.type=="Accepted")].status}{"\n"}'

# 控制器本身是一个 Deployment，加一个 webhook Service。
kubectl -n nginx-gateway get deploy,svc

# 从日志确认控制器启动并开始 watch Gateway API 资源。
kubectl -n nginx-gateway logs deploy/ngf-nginx-gateway-fabric --tail=8
