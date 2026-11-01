#!/usr/bin/env bash

#> expect: kube-scheduler
#> expect: kube-controller-manager
#> expect: apiserver-
kubectl get lease -n kube-system

#> expect: HOLDER
#> expect: 15
kubectl get lease -n kube-system kube-scheduler kube-controller-manager \
  -o custom-columns='NAME:.metadata.name,HOLDER:.spec.holderIdentity,RENEW:.spec.renewTime,DURATION:.spec.leaseDurationSeconds'

#> expect: shorty-control-plane
kubectl get lease -n kube-node-lease
