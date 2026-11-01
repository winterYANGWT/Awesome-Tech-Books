# 在集群里起一个带 wget/ping/nslookup 的客户端。
kubectl -n shorty run client --image=busybox:1.37 --image-pull-policy=IfNotPresent --restart=Never --command -- sleep 3600
kubectl -n shorty wait --for=condition=Ready pod/client --timeout=120s

# ClusterIP：只有一个集群内虚拟 IP，没有 External-IP；-o wide 带出 SELECTOR。
kubectl -n shorty get svc api -o wide

# 在 Pod 里按 Service 名访问，走的是 8080 端口。
kubectl -n shorty exec client -- wget -qO- http://api:8080/healthz

# 同一个 ClusterIP 用 ping 不通：它不是任何一台主机上的地址。
#> fails
#> expect: (100% packet loss|timed out|Network is unreachable)
kubectl -n shorty exec client -- ping -c 1 -W 2 api

# 同一批后端再加一个 NodePort 和一个 LoadBalancer 入口。
kubectl apply --request-timeout=300s -f api-nodeport.yaml
kubectl apply --request-timeout=300s -f api-lb.yaml

# LoadBalancer 的地址由 cloud-provider-kind 异步写进 .status.loadBalancer.ingress。
#> hide
for i in $(seq 1 90); do
    lb_ip=$(kubectl -n shorty get svc api-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
    [ -n "$lb_ip" ] && break
    sleep 1
done

# 三个 Service 的 TYPE、CLUSTER-IP、EXTERNAL-IP 和 PORT(S) 放在一起看。
kubectl -n shorty get svc api api-nodeport api-lb

# LoadBalancer 的 IP 也可以从 lab 直接访问，后端仍然是 api 的两个 Pod。
printf 'loadbalancer=%s\n' "$lb_ip"
#> hide
for i in $(seq 1 30); do
    curl -sS --max-time 2 "http://$lb_ip:8080/healthz" >/dev/null 2>&1 && break
    sleep 1
done
curl -sS --max-time 5 "http://$lb_ip:8080/healthz"
