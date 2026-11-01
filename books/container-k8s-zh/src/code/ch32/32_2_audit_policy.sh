#> expect: --audit-policy-file
#> expect: --audit-log-path
# 审计配置是建集群时通过 kubeadm 的 extraArgs 写进静态 Pod 清单的。
docker exec shorty-control-plane grep -E 'audit-(policy-file|log-path|log-maxage|log-maxbackup|log-maxsize)' \
  /etc/kubernetes/manifests/kube-apiserver.yaml

#> expect: audit.log
# 日志目录从 lab 挂进节点，不放在节点容器的可写层里。
ls -l /tmp/ch32-audit-log/audit.log

# 发一个读取 Secret 的请求。
kubectl -n shorty get secret shorty-db -o jsonpath='{.metadata.name}{"\n"}'

#> expect: "user":"kubernetes-admin"
#> expect: "verb":"get"
#> expect: "code":200
# 在审计日志里找这条请求：谁、做了什么、对哪个对象、响应码是多少。
jq -c 'select(.objectRef.resource == "secrets" and .objectRef.name == "shorty-db")
       | {user: .user.username, verb, resource: .objectRef.resource,
          name: .objectRef.name, code: .responseStatus.code}' \
  /tmp/ch32-audit-log/audit.log | tail -n 1

#> expect: "hasRequest":false
#> expect: "hasResponse":false
# Metadata 级别只记元数据：请求体和响应体都不在日志里。
jq -c 'select(.objectRef.resource == "secrets" and .objectRef.name == "shorty-db")
       | {level, hasRequest: (.requestObject != null), hasResponse: (.responseObject != null)}' \
  /tmp/ch32-audit-log/audit.log | tail -n 1
