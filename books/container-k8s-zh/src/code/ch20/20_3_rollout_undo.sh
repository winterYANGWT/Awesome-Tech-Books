kubectl -n shorty annotate deployment/api kubernetes.io/change-cause="bad image shorty:0.9" --overwrite
kubectl -n shorty set image deployment/api api=shorty:0.9
#> fails
#> expect: exceeded its progress deadline
#> timeout: 120
kubectl -n shorty rollout status deployment/api --timeout=90s
#> expect: ErrImagePull|ImagePullBackOff
kubectl -n shorty get pods -l app.kubernetes.io/name=api
#> expect: ProgressDeadlineExceeded
kubectl -n shorty describe deployment api | sed -n '/^Conditions:/,/^OldReplicaSets:/p'
kubectl -n shorty rollout history deployment/api
kubectl -n shorty rollout undo deployment/api
kubectl -n shorty wait --for=condition=Progressing deployment/api --timeout=180s
kubectl -n shorty rollout status deployment/api --timeout=60s
kubectl -n shorty get pods -l app.kubernetes.io/name=api
# restart 不换镜像，只给 Pod 模板加一个重启时间注解，也会生成新的 RS。
kubectl -n shorty rollout restart deployment/api
kubectl -n shorty rollout status deployment/api --timeout=180s
kubectl -n shorty get deployment api -o jsonpath='{.spec.template.spec.containers[0].image}{"\n"}'
