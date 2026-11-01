# 36.5 证书检查。check-expiration 列出 /etc/kubernetes/pki 下的证书和 kubeconfig 内嵌证书。
docker exec shorty-control-plane kubeadm certs check-expiration

#> expect: 30 天内到期: 0 项
# RESIDUAL TIME 是 364d 这样的格式；筛出 30 天以内的项，没有就报 0。
docker exec shorty-control-plane kubeadm certs check-expiration 2>/dev/null \
  | awk 'NR>1 && $3 ~ /^[0-9]+d$/ {d=$3; sub("d","",d); if (d+0 <= 30) {print; n++}} END {printf "30 天内到期: %d 项\n", n+0}'

# 续期前记下 apiserver 证书的序列号和到期时间。
docker exec shorty-control-plane openssl x509 -in /etc/kubernetes/pki/apiserver.crt \
  -noout -serial -enddate

#> expect: renewed
# kubeadm 用 /etc/kubernetes/pki 里的 CA 重新签发；多控制面要逐台执行。
docker exec shorty-control-plane kubeadm certs renew all

# 序列号变了，说明文件确实换了；notAfter 也往后推了一年。
docker exec shorty-control-plane openssl x509 -in /etc/kubernetes/pki/apiserver.crt \
  -noout -serial -enddate
docker exec shorty-control-plane kubeadm certs check-expiration 2>/dev/null \
  | grep -E '^(apiserver|admin.conf)'
