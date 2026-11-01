# 36.3 快照恢复演练。先记下快照时刻之后创建的对象：恢复后它应该消失。
kubectl -n shorty create configmap post-snapshot-marker \
  --from-literal=created=after-snapshot

ETCD_ID=$(docker exec shorty-control-plane crictl ps --name etcd -q)
# 静态 Pod 清单里的 peer URL 随节点 IP 变化，恢复时要把同一个值传给 etcdutl。
PEER=$(docker exec shorty-control-plane \
  grep -m1 -- '--initial-cluster=' /etc/kubernetes/manifests/etcd.yaml | sed 's/.*=//')
echo "$PEER"

#> timeout: 300
# 恢复顺序照官方文档：先停 apiserver、controller-manager、scheduler。
# 静态 Pod 由 kubelet 看清单目录管理，把清单移走，kubelet 就会停掉容器。
docker exec shorty-control-plane sh -c '
  mkdir -p /tmp/ch36-manifests
  mv /etc/kubernetes/manifests/kube-apiserver.yaml \
     /etc/kubernetes/manifests/kube-controller-manager.yaml \
     /etc/kubernetes/manifests/kube-scheduler.yaml \
     /tmp/ch36-manifests/'
for i in $(seq 1 30); do
  if [ -z "$(docker exec shorty-control-plane crictl ps --name kube-apiserver -q)" ]; then break; fi
  sleep 2
done
docker exec shorty-control-plane crictl ps --name kube-apiserver

#> timeout: 300
# 恢复到新的 data-dir；--name、--initial-cluster、--initial-advertise-peer-urls
# 要和 etcd 静态 Pod 清单一致。写到 /var/lib/etcd 的子目录，避免碰还在用的旧数据。
docker exec shorty-control-plane crictl exec "$ETCD_ID" \
  /usr/local/bin/etcdutl \
  --data-dir /var/lib/etcd/restored \
  --name shorty-control-plane \
  --initial-cluster "shorty-control-plane=$PEER" \
  --initial-advertise-peer-urls "$PEER" \
  snapshot restore /etc/kubernetes/pki/etcd/snapshot.db 2>&1 | grep 'restored snapshot'

#> timeout: 300
# 再停 etcd，把它清单里的 data-dir 换成恢复出来的目录，然后重新拉起 etcd。
docker exec shorty-control-plane sh -c '
  mv /etc/kubernetes/manifests/etcd.yaml /tmp/ch36-manifests/'
for i in $(seq 1 30); do
  if [ -z "$(docker exec shorty-control-plane crictl ps --name etcd -q)" ]; then break; fi
  sleep 2
done
docker exec shorty-control-plane sh -c '
  sed -i "s#path: /var/lib/etcd\$#path: /var/lib/etcd/restored#" /tmp/ch36-manifests/etcd.yaml
  grep "path: /var/lib/etcd" /tmp/ch36-manifests/etcd.yaml'
docker exec shorty-control-plane sh -c '
  mv /tmp/ch36-manifests/etcd.yaml /etc/kubernetes/manifests/'
for i in $(seq 1 60); do
  if [ -n "$(docker exec shorty-control-plane crictl ps --name etcd -q)" ]; then break; fi
  sleep 2
done
docker exec shorty-control-plane crictl ps --name etcd

#> timeout: 300
# etcd 起来后再把三个控制面组件放回去，等 apiserver 的 /readyz 返回 ok。
docker exec shorty-control-plane sh -c '
  mv /tmp/ch36-manifests/kube-apiserver.yaml \
     /tmp/ch36-manifests/kube-controller-manager.yaml \
     /tmp/ch36-manifests/kube-scheduler.yaml \
     /etc/kubernetes/manifests/'
for i in $(seq 1 90); do
  if kubectl get --raw=/readyz >/dev/null 2>&1; then break; fi
  sleep 2
done
kubectl get --raw=/readyz; echo

kubectl get nodes

#> fails
#> expect: NotFound
# 快照之后创建的 ConfigMap 不在了，说明对象回到了拍快照的那一刻。
kubectl -n shorty get configmap post-snapshot-marker

kubectl -n shorty get pods -o wide
