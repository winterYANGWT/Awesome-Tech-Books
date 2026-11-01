kubectl -n shorty apply -f node-agent.yaml
kubectl -n shorty rollout status daemonset/node-agent --timeout=180s
kubectl -n shorty get daemonset node-agent
kubectl -n shorty get pods -l app.kubernetes.io/name=node-agent -o wide
kubectl -n shorty get daemonset node-agent -o jsonpath='updateStrategy={.spec.updateStrategy.type} maxUnavailable={.spec.updateStrategy.rollingUpdate.maxUnavailable}{"\n"}'
kubectl -n shorty get daemonset node-agent -o jsonpath='{.spec.template.spec.tolerations}{"\n"}'
kubectl -n shorty get pods -l app.kubernetes.io/name=node-agent -o jsonpath='{range .items[*]}{.spec.nodeName}{" scheduler="}{.spec.schedulerName}{" affinity="}{.spec.affinity.nodeAffinity.requiredDuringSchedulingIgnoredDuringExecution.nodeSelectorTerms[0].matchFields[0].key}{"="}{.spec.affinity.nodeAffinity.requiredDuringSchedulingIgnoredDuringExecution.nodeSelectorTerms[0].matchFields[0].values[0]}{"\n"}{end}'
