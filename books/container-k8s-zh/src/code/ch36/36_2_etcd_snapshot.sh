# 36.2 etcd 快照。先找到控制面节点里 etcd 静态 Pod 的容器 ID；
# etcd 镜像里带 etcdctl 和 etcdutl，可以直接用 crictl exec 调用。
ETCD_ID=$(docker exec shorty-control-plane crictl ps --name etcd -q)
echo "$ETCD_ID"

#> expect: Snapshot saved
#> tail: 4
# --endpoints 用回环地址，证书用静态 Pod 挂进去的那三份。
docker exec shorty-control-plane crictl exec "$ETCD_ID" \
  /usr/local/bin/etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /etc/kubernetes/pki/etcd/snapshot.db

#> expect: HASH
#> expect: TOTAL KEYS
# 校验快照：etcdctl snapshot status 从 3.5 起废弃，这里用 etcdutl。
docker exec shorty-control-plane crictl exec "$ETCD_ID" \
  /usr/local/bin/etcdutl --write-out=table \
  snapshot status /etc/kubernetes/pki/etcd/snapshot.db

# 把快照复制出集群，算出摘要；真实环境要再传到集群外的对象存储并加密。
docker cp shorty-control-plane:/etc/kubernetes/pki/etcd/snapshot.db \
  /tmp/ch36-etcd-snapshot.db
ls -lh /tmp/ch36-etcd-snapshot.db
sha256sum /tmp/ch36-etcd-snapshot.db
