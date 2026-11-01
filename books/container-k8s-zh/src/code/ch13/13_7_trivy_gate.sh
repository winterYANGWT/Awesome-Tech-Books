# 13.7：用 2019 年的 alpine:3.10 触发 CRITICAL，门禁退出 1；换 3.22 后通过
TRIVY="docker run --rm -v /var/run/docker.sock:/var/run/docker.sock -v /book/build/scratch/ch13/trivy-cache:/root/.cache/trivy aquasec/trivy:0.76.1"

#> fails
#> expect: CRITICAL: [1-9]
#> expect: CVE-2021-36159
$TRIVY image --quiet --scanners vuln --severity HIGH,CRITICAL --exit-code 1 \
  --image-src docker alpine:3.10

#> expect: alpine 3.22
$TRIVY image --quiet --scanners vuln --severity HIGH,CRITICAL --exit-code 1 \
  --image-src docker alpine:3.22
