#!/usr/bin/env bash

#> expect: /api/v1/namespaces/shorty/pods
#> expect: 201 Created
kubectl apply -v=6 -f trace-pod.yaml 2>&1 \
  | grep -E 'GET.*/pods/trace-demo|POST.*/pods|trace-demo created'

kubectl wait --for=condition=Ready pod/trace-demo -n shorty --timeout=60s
kubectl get pod trace-demo -n shorty -o wide

#> expect: Scheduled
#> expect: Started
kubectl get events -n shorty --field-selector involvedObject.name=trace-demo \
  --sort-by=.lastTimestamp \
  -o custom-columns='TIME:.lastTimestamp,TYPE:.type,REASON:.reason,SOURCE:.source.component,MESSAGE:.message'

kubectl delete pod trace-demo -n shorty
