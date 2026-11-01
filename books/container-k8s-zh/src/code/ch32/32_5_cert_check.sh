#> expect: apiserver
#> expect: etcd-server
# 在控制面节点里查 kubeadm 管理的证书到期时间。
docker exec shorty-control-plane kubeadm certs check-expiration

#> expect: rotateCertificates: true
#> expect: kubelet-client-current.pem
# kubelet 的客户端证书不在上面的列表里，它由 kubelet 自己轮换。
docker exec shorty-worker sh -c \
  'grep -E "rotateCertificates|serverTLSBootstrap" /var/lib/kubelet/config.yaml; ls -l /var/lib/kubelet/pki'
