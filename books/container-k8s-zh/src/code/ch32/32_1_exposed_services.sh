# 第 23 章加过的两种对外入口；s4-prod 只保留主链路，这里补回来。
kubectl apply -f exposed-services.yaml

#> expect: api-nodeport
#> expect: api-lb
# 全集群看 Service：NodePort 会分到 30000–32767 的端口，LoadBalancer 等外部实现给地址。
kubectl get svc -A -o wide

#> expect: api-nodeport\tNodePort\t3[0-9]+
#> expect: api-lb\tLoadBalancer\t3[0-9]+
# 只列对外暴露的类型，NodePort 端口每次分配可能不同。
kubectl get svc -A -o json \
  | jq -r '.items[] | select(.spec.type == "NodePort" or .spec.type == "LoadBalancer")
           | [.metadata.namespace, .metadata.name, .spec.type,
              ([.spec.ports[].nodePort] | join(","))] | @tsv'

#> expect: allow-gateway
# 谁在监听、从哪来：策略按 Pod 标签放行，不看 Service 类型。
kubectl get networkpolicy -A

#> expect: 8080
# Gateway 这条入口只放行 8080，且来源限定在 shorty 内或 Gateway 数据面。
kubectl -n shorty describe networkpolicy allow-gateway
