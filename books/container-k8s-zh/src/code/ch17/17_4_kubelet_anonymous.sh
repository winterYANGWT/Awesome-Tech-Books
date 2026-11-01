#!/usr/bin/env bash

#> expect: enabled: false
docker exec shorty-control-plane \
  sed -n '/^authentication:/,/^cgroupDriver:/p' /var/lib/kubelet/config.yaml

#> hide
NODE_IP=$(kubectl get node shorty-control-plane \
  -o jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')

#> expect: HTTP 401
curl -k -s -o /dev/null -w 'HTTP %{http_code}\n' --max-time 5 \
  "https://$NODE_IP:10250/healthz"

#> expect: Unauthorized
curl -k -s --max-time 5 "https://$NODE_IP:10250/healthz"

#> expect: shorty
kubectl exec -n shorty deploy/api -- /shorty version
