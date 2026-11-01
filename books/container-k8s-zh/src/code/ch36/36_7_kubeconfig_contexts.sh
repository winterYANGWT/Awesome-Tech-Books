# 36.7 多集群在客户端就是 kubeconfig 里的 context：cluster、user、namespace 三元组。
cp ~/.kube/config /tmp/ch36-kubeconfig
kubectl --kubeconfig /tmp/ch36-kubeconfig config get-contexts

# 加一个指向本机 1 端口的 cluster 和一个使用它的 context。
kubectl --kubeconfig /tmp/ch36-kubeconfig config set-cluster unreachable \
  --server=https://127.0.0.1:1 --insecure-skip-tls-verify=true
kubectl --kubeconfig /tmp/ch36-kubeconfig config set-context unreachable \
  --cluster=unreachable --user=kind-shorty
kubectl --kubeconfig /tmp/ch36-kubeconfig config use-context unreachable

#> fails
#> expect: connection refused
kubectl --kubeconfig /tmp/ch36-kubeconfig get nodes

# 切回 kind-shorty，同一个 kubeconfig 又能访问真正存在的集群。
kubectl --kubeconfig /tmp/ch36-kubeconfig config use-context kind-shorty
kubectl --kubeconfig /tmp/ch36-kubeconfig get nodes
