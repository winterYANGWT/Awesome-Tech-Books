# 31.3 先建策略。策略本身只是逻辑，绑定才决定它作用在哪些请求上。
kubectl apply -f vap-require-team-label.yaml

# 等 apiserver 把这个策略编译进准入链。直接读 status.observedGeneration，
# 比固定 sleep 可靠：观察到的 generation 到了才建绑定，否则绑定可能先到、找不到策略。
kubectl wait --for=jsonpath='{.status.observedGeneration}'=1 validatingadmissionpolicy/require-team-label --timeout=60s
kubectl apply -f vap-require-team-label-binding.yaml

# 绑定注册到准入链也是异步的；用 server-side dry-run 等到这条策略真的开始拦截，
# 避免第一次 apply 撞在注册完成之前。dry-run 不落盘，不会真的创建 Pod。
#> hide
for i in $(seq 1 30); do
  if kubectl apply --dry-run=server -f vap-no-label.yaml >/dev/null 2>&1; then
    sleep 1
  else
    break
  fi
done

# 没有 team 标签的 Pod 被拒绝；错误里带着策略名、绑定名和自定义 message。
#> fails
#> expect: ValidatingAdmissionPolicy 'require-team-label' with binding 'require-team-label-binding' denied request
#> expect: 'team' label is required
kubectl apply -f vap-no-label.yaml

# 带上 team 标签的同一个 Pod 通过 PSA 和 VAP，正常创建。
kubectl apply -f vap-with-label.yaml
kubectl -n shorty wait --for=condition=Ready pod/vap-with-label --timeout=60s
kubectl -n shorty get pod vap-with-label --show-labels

# 清理这个演示 Pod；策略和绑定留着，31.4 要用它验证 MAP 注入的标签能让 VAP 通过。
kubectl -n shorty delete pod vap-with-label
