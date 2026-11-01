#!/usr/bin/env bash

#> expect: kube-apiserver
#> expect: etcd
#> expect: kube-scheduler
#> expect: kube-controller-manager
kubectl get pods -n kube-system -o wide
