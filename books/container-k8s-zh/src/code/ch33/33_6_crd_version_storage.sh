# 给 CRD 加 v1，并把 v1 设为 storage；旧对象仍可按 v1alpha1 或 v1 读。
kubectl apply -f crd-v1.yaml

kubectl get crd shortybackups.shorty.example.com -o jsonpath='{range .spec.versions[*]}{.name} served={.served} storage={.storage}{"\n"}{end}'

# 等 storedVersions 里出现 v1；这是 CRD 控制器异步写的。
#> hide
for i in $(seq 1 30); do
  kubectl get crd shortybackups.shorty.example.com -o jsonpath='{.status.storedVersions}' 2>/dev/null | grep -q v1 && break
  sleep 1
done

kubectl get crd shortybackups.shorty.example.com -o jsonpath='{.status.storedVersions}{"\n"}'

# kubectl 的 discovery 缓存可能还停在 v1alpha1；换一个 cache 目录强制重新发现。
kubectl --cache-dir=/tmp/ch33-kube-cache-$$ -n shorty get shortybackup shorty-backup -o jsonpath='{.apiVersion} phase={.status.phase}{"\n"}'

kubectl --cache-dir=/tmp/ch33-kube-cache-$$ -n shorty get shortybackup.v1alpha1.shorty.example.com shorty-backup -o jsonpath='{.apiVersion} phase={.status.phase}{"\n"}'
