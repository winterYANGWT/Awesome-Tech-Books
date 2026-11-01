kubectl -n shorty apply -f pod_share_network.yaml
kubectl -n shorty wait --for=condition=Ready pod/share-net --timeout=120s
kubectl -n shorty get pod share-net -o wide

# 两个容器看到的 hostname 一样：它们共享 UTS namespace。
for c in server client; do
    printf '%-8s hostname: ' "$c"
    kubectl -n shorty exec share-net -c "$c" -- hostname
done

# client 用 localhost 访问 server 的 8080；Pod 内共享 network namespace。
kubectl -n shorty exec share-net -c client -- wget -qO- http://localhost:8080/

# 两个容器的 PID namespace 是分开的：各自的 PID 1 是各自的进程。
for c in server client; do
    echo "--- $c"
    kubectl -n shorty exec share-net -c "$c" -- ps -o pid,args
done

# 节点上，Pod 的 sandbox（pause）容器持有 network namespace。
# 取 sandbox 和两个容器的 PID，比较 /proc/<pid>/ns/net 是否指向同一个 namespace。
node=shorty-control-plane
pod_id=$(docker exec "$node" crictl pods --name share-net -q)
sandbox_pid=$(docker exec "$node" crictl inspectp -o go-template --template '{{.info.pid}}' "$pod_id")
sandbox_net=$(docker exec "$node" readlink "/proc/$sandbox_pid/ns/net")
printf 'sandbox  pid=%-6s netns=%s\n' "$sandbox_pid" "$sandbox_net"
for c in server client; do
    cid=$(docker exec "$node" crictl ps --pod "$pod_id" --name "^$c$" -q)
    pid=$(docker exec "$node" crictl inspect -o go-template --template '{{.info.pid}}' "$cid")
    net=$(docker exec "$node" readlink "/proc/$pid/ns/net")
    printf '%-8s pid=%-6s netns=%s\n' "$c" "$pid" "$net"
done
[ "$sandbox_net" = "$net" ] && echo "sandbox 与两个容器在同一个 network namespace"
