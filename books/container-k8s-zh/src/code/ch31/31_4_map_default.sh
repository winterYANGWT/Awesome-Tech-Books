# 31.4 建变更策略和绑定。它和 VAP 一样是进程内准入，不需要 webhook 服务。
kubectl apply -f map-default-team.yaml
kubectl apply -f map-default-team-binding.yaml

# MAP 的绑定同样要等控制器注册；server-side dry-run 只走准入，不落盘。
# 看到 dry-run 的结果里出现 team=unknown，说明 MAP 已经生效。
#> hide
for i in $(seq 1 30); do
  team=$(kubectl apply --dry-run=server -f vap-no-label.yaml -o jsonpath='{.metadata.labels.team}' 2>/dev/null || true)
  [ "$team" = unknown ] && break
  sleep 1
done

# 31.3 里被 VAP 拒绝的同一个 Pod，这次没有 team 标签。
# MAP 在 validating 阶段之前先注入 team，VAP 随后看到的就是有标签的对象。
kubectl apply -f vap-no-label.yaml
kubectl -n shorty wait --for=condition=Ready pod/vap-no-label --timeout=60s

# 看注入结果：team=unknown 是 MAP 加的，app 是清单里原有的，没有被覆盖掉。
kubectl -n shorty get pod vap-no-label -o jsonpath='{.metadata.labels}{"\n"}'
kubectl -n shorty get pod vap-no-label -o jsonpath='team={.metadata.labels.team}{"\n"}'

# 清理演示 Pod，策略和绑定留给 31.5 的 userns Pod 用。
kubectl -n shorty delete pod vap-no-label
