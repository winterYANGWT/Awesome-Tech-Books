# 34.8 排障：路径写错时 Argo CD 报 ComparisonError，先看 condition 和 repo-server。
kubectl apply -f bad-application.yaml
for _ in $(seq 1 30); do
    cond=$(kubectl -n argocd get application shorty-bad \
        -o jsonpath='{range .status.conditions[*]}{.type}{"\n"}{end}' 2>/dev/null)
    case "$cond" in *ComparisonError*) break ;; esac
    sleep 2
done

kubectl -n argocd get application shorty-bad \
    -o jsonpath='{range .status.conditions[*]}{.type}: {.message}{"\n"}{end}'
kubectl -n argocd logs deploy/argocd-repo-server --tail=50 \
    | grep -iE 'error|not found|does not exist' | tail -8
kubectl -n argocd delete application shorty-bad

# Helm 发现同名对象已有别的 release 归属，会拒绝接管；不要随手 --take-ownership。
#> fails
#> expect: (already exists|cannot be imported)
helm install api ./chart -n shorty --wait --timeout 30s
helm uninstall api -n shorty >/dev/null 2>&1 || true

# patch 的 target 写错 namespace 时，Kustomize 不会改那个对象；对比正确 target 的渲染结果。
kubectl kustomize kustomize/badpatch \
    | yq eval-all -r 'select(.kind == "Deployment") | "badpatch replicas=\(.spec.replicas)"'
kubectl kustomize kustomize/goodpatch \
    | yq eval-all -r 'select(.kind == "Deployment") | "goodpatch replicas=\(.spec.replicas)"'
