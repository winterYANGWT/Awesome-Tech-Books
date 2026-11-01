# 同一个 ConfigMap 分别用“目录卷”和“subPath 文件”挂载，同时也注入环境变量。
kubectl apply -f configmap-hot.yaml
kubectl -n shorty wait --for=condition=Ready pod/cm-dir pod/cm-subpath --timeout=120s
kubectl -n shorty exec cm-dir -- sh -c 'echo "dir file: $(cat /etc/shorty-runtime/app.properties | tr "\n" ",")"; echo "dir env: $(printenv GREETING)"'
kubectl -n shorty exec cm-subpath -- sh -c 'echo "subPath file: $(cat /etc/shorty-runtime/app.properties | tr "\n" ",")"'

# 改 ConfigMap 的内容。
kubectl -n shorty patch configmap shorty-runtime --type=merge \
  -p '{"data":{"GREETING":"hello-v2","app.properties":"mode=v2\nbuild=2\n"}}'

# 更新 Pod 注解，让 kubelet 尽快做一次同步；然后等目录卷里的文件变成新值。
#> hide
kubectl -n shorty annotate pod cm-dir refresh="$(date +%s)" --overwrite >/dev/null
#> hide
kubectl -n shorty annotate pod cm-subpath refresh="$(date +%s)" --overwrite >/dev/null
#> hide
#> timeout: 300
for i in $(seq 1 120); do
  kubectl -n shorty exec cm-dir -- cat /etc/shorty-runtime/app.properties 2>/dev/null | grep -q 'mode=v2' && break
  sleep 2
done

# 目录卷最终变了；环境变量和 subPath 都还是旧值。
#> expect: dir file: mode=v2,build=2,
#> expect: dir env: hello-v1
kubectl -n shorty exec cm-dir -- sh -c 'echo "dir file: $(cat /etc/shorty-runtime/app.properties | tr "\n" ",")"; echo "dir env: $(printenv GREETING)"'

#> expect: subPath file: mode=v1,build=1,
kubectl -n shorty exec cm-subpath -- sh -c 'echo "subPath file: $(cat /etc/shorty-runtime/app.properties | tr "\n" ",")"'
