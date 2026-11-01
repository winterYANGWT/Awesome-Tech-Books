# 1) schema 里没有 compress：多余字段被 apiserver 的 strict decoding 拒绝。
#> fails
#> expect: strict decoding error: unknown field "spec.compress"
kubectl apply -f cr-bad-extra.yaml

# 2) 类型不对：这个字段在 schema 里存在，但 retentionDays 要 integer。
#> fails
#> expect: spec.retentionDays in body must be of type integer
kubectl apply -f cr-bad-type.yaml

# 3) 把 compress 加进 schema，再创建同一个对象；不写 retentionDays，看 default。
kubectl apply -f crd-v1alpha1-compress.yaml

kubectl apply -f cr-compress.yaml

kubectl -n shorty get shortybackup shorty-backup-bad -o jsonpath='retentionDays={.spec.retentionDays} compress={.spec.compress}{"\n"}'

kubectl -n shorty get shortybackup shorty-backup-bad

# 这个 CR 只是 schema 演示用的；删掉，不让它进入后面的 reconcile。
kubectl -n shorty delete shortybackup shorty-backup-bad
