# shorty-db 是 Opaque 类型，data 里的密码是 base64 编码的字符串。
kubectl -n shorty get secret shorty-db -o jsonpath='{.type}{"\n"}{.data.db_password}{"\n"}'
kubectl -n shorty get secret shorty-db -o jsonpath='{.data.db_password}' | base64 -d | wc -c

# 用 kubectl create secret generic 建一个临时 Secret，看 data 和 stringData 的关系。
kubectl -n shorty create secret generic demo-login \
  --from-literal=username=admin --from-literal=password='S3cret!'
kubectl -n shorty get secret demo-login -o jsonpath='{.data.password}{"\n"}'
kubectl -n shorty get secret demo-login -o jsonpath='{.data.password}' | base64 -d; echo
kubectl -n shorty delete secret demo-login

# api 的 Secret 卷挂到 /etc/shorty，权限 0444（十进制 292）。
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.containers[0].volumeMounts[0].mountPath}{" "}{.spec.template.spec.volumes[0].secret.secretName}{" "}{.spec.template.spec.volumes[0].secret.defaultMode}{"\n"}'

# 用一个同样挂载方式的 busybox Pod 观察容器内视角：只有文件路径，没有明文密码。
kubectl apply -f secret-reader.yaml
kubectl -n shorty wait --for=condition=Ready pod/secret-reader --timeout=120s
kubectl -n shorty exec secret-reader -- sh -c '
echo "PASSWORD 相关变量："; env | grep PASSWORD || true
echo "明文密码变量："; env | grep "^SHORTY_DB_PASSWORD=" || echo "（没有）"
echo "密码文件字节数：$(wc -c < /etc/shorty/db_password)"
echo "密码文件权限：$(ls -lL /etc/shorty/db_password | cut -c1-10)"
echo "挂载类型：$(df -h /etc/shorty | tail -n 1 | sed "s/  */ /g" | cut -d" " -f1)"
'
