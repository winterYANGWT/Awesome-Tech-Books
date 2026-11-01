# 36.8 排障演示。先造一个 1 副本、PDB minAvailable=1 的 Deployment。
kubectl apply -f pdb-demo.yaml
kubectl -n shorty rollout status deployment/pdb-demo --timeout=120s
PDB_NODE=$(kubectl -n shorty get pod -l app=pdb-demo \
  -o jsonpath='{.items[0].spec.nodeName}')
echo "pdb-demo 在 $PDB_NODE"

#> fails
#> expect: disruption budget
#> timeout: 120
# 唯一副本不能动，drain 会在 PDB 这一关被挡住；节点已经被 cordon。
kubectl drain "$PDB_NODE" --ignore-daemonsets --delete-emptydir-data --timeout=15s

# 先看 PDB 和副本：minAvailable=1 而只有 1 个副本，任何驱逐都会违反预算。
kubectl -n shorty get pdb pdb-demo
kubectl -n shorty get deployment pdb-demo

# 演示环境里把 PDB 临时放宽到 0；生产更稳妥的做法是先加副本，再 drain。
kubectl -n shorty patch pdb pdb-demo --type=merge -p '{"spec":{"minAvailable":0}}'
#> expect: drained
kubectl drain "$PDB_NODE" --ignore-daemonsets --delete-emptydir-data --timeout=120s
kubectl uncordon "$PDB_NODE"
kubectl -n shorty delete -f pdb-demo.yaml

#> fails
#> expect: ignore-daemonsets
#> timeout: 60
# 不带 --ignore-daemonsets，kube-proxy/kindnet 这类 DaemonSet Pod 会让 drain 报错。
kubectl drain "$PDB_NODE" --delete-emptydir-data --timeout=15s
kubectl uncordon "$PDB_NODE"

# 再试 emptyDir：把 Pod 固定到一个 worker 上，drain 时默认拒绝删本地数据。
PG_NODE=$(kubectl -n shorty get pod -l app.kubernetes.io/name=postgres \
  -o jsonpath='{.items[0].spec.nodeName}')
if [ "$PG_NODE" = shorty-worker ]; then TARGET=shorty-worker2; else TARGET=shorty-worker; fi
kubectl apply -f emptydir-demo.yaml
kubectl -n shorty patch deployment emptydir-demo --type=merge \
  -p "{\"spec\":{\"template\":{\"spec\":{\"nodeSelector\":{\"kubernetes.io/hostname\":\"$TARGET\"}}}}}"
kubectl -n shorty rollout status deployment/emptydir-demo --timeout=120s

#> fails
#> expect: delete-emptydir-data
#> timeout: 60
kubectl drain "$TARGET" --ignore-daemonsets --timeout=15s

#> expect: drained
# 确认 emptyDir 里的数据可以丢之后，再加 --delete-emptydir-data。
kubectl drain "$TARGET" --ignore-daemonsets --delete-emptydir-data --timeout=120s
kubectl uncordon "$TARGET"
kubectl -n shorty rollout status deployment/emptydir-demo --timeout=120s
kubectl -n shorty delete -f emptydir-demo.yaml

# etcd 恢复的常见错误：目标 data-dir 非空。换一个新目录，或先停 etcd 再清空。
ETCD_ID=$(docker exec shorty-control-plane crictl ps --name etcd -q)
#> fails
#> expect: not empty
docker exec shorty-control-plane crictl exec "$ETCD_ID" \
  /usr/local/bin/etcdutl --data-dir /var/lib/etcd \
  snapshot restore /etc/kubernetes/pki/etcd/snapshot.db

# apiserver 起不来时，先看它的 --etcd-servers/证书参数和 etcd 数据目录的权限。
docker exec shorty-control-plane sh -c '
  grep -E "etcd-servers|etcd-cafile|etcd-certfile" /etc/kubernetes/manifests/kube-apiserver.yaml
  ls -ld /var/lib/etcd'
