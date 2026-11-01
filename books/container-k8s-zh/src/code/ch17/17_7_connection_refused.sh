#!/usr/bin/env bash

#> fails
#> expect: was refused
kubectl --server=https://127.0.0.1:1 get pods

kind get clusters

#> expect: shorty-control-plane
docker ps --filter name=shorty-control-plane --format '{{.Names}}  {{.Status}}'

docker logs shorty-control-plane --tail 8
