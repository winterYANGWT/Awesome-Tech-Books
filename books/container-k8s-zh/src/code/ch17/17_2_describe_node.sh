#!/usr/bin/env bash

#> head: 78
#> expect: Capacity:
#> expect: Allocatable:
#> expect: KubeletReady
kubectl describe node shorty-control-plane

#> fails
#> expect: Metrics API not available
kubectl top node
