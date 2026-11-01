# kube-proxy 的模式写在 kube-system/kube-proxy 这个 ConfigMap 里。
kubectl -n kube-system get configmap kube-proxy -o go-template='{{index .data "config.conf"}}'

# 每个节点一个 kube-proxy Pod。
kubectl -n kube-system get pods -l k8s-app=kube-proxy -o wide

# 启动日志会说明实际选了哪个 Proxier。
kubectl -n kube-system logs -l k8s-app=kube-proxy --tail=200 | awk '/Using .* Proxier/ && ++n <= 3'

# 找到 api 的 ClusterIP 对应的 iptables 跳转链。
cluster_ip=$(kubectl -n shorty get svc api -o jsonpath='{.spec.clusterIP}')
svc_chain=$(docker exec shorty-worker iptables-save -t nat | awk -v ip="$cluster_ip" 'index($0,"-d " ip "/32") && !found {for (i=1; i<=NF; i++) if ($i ~ /^KUBE-SVC-/) {print $i; found=1}}')
printf 'clusterIP=%s chain=%s\n' "$cluster_ip" "$svc_chain"
docker exec shorty-worker iptables-save -t nat | grep -- "-d $cluster_ip/32"

# 这条链在每个后端之间随机选一个，再跳到各自的 KUBE-SEP 链。
docker exec shorty-worker iptables-save -t nat | grep -A3 -- "-A $svc_chain"

# KUBE-SEP 链做 DNAT：把目的地址改成 Pod IP。
for ip in $(kubectl -n shorty get pods -l app.kubernetes.io/name=api -o jsonpath='{range .items[*]}{.status.podIP}{"\n"}{end}'); do
    docker exec shorty-worker iptables-save -t nat | awk -v ip="$ip" '/^-A KUBE-SEP/ && index($0,"to-destination " ip ":8080")'
done
