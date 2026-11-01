# 11.4：先记录 Trivy 和漏洞库的版本，扫描结果才有可复现的上下文
#> head: 12
#> expect: Vulnerability DB
docker run --rm -v ch11-trivy-cache:/root/.cache/trivy \
    aquasec/trivy:0.76.1 version

# 扫描 scratch 镜像。--exit-code 0 只统计，不阻断；门禁时改成 1。
#> head: 40
#> expect: Total:
docker run --rm -e NO_COLOR=1 \
    -v /var/run/docker.sock:/var/run/docker.sock \
    -v ch11-trivy-cache:/root/.cache/trivy \
    aquasec/trivy:0.76.1 image --scanners vuln \
    --severity HIGH,CRITICAL --skip-db-update --exit-code 0 shorty:0.1

# 同样扫 worker 的 python slim 基础镜像，只留目标名和 Total 行
docker run --rm -e NO_COLOR=1 \
    -v /var/run/docker.sock:/var/run/docker.sock \
    -v ch11-trivy-cache:/root/.cache/trivy \
    aquasec/trivy:0.76.1 image --scanners vuln \
    --severity HIGH,CRITICAL --skip-db-update --exit-code 0 shorty-worker:0.1 \
    2>&1 | grep -E '^(shorty|Python|Total:)' | head -20

# 报告里要带基础镜像 digest：worker 的 base 是 python slim
#> expect: python@sha256:
docker image inspect --format '{{index .RepoDigests 0}}' python:3.15.2-slim-trixie

# 门禁：同样的扫描加 --exit-code 1，有发现就以非 0 退出
#> expect: trivy exit=1
docker run --rm -e NO_COLOR=1 \
    -v /var/run/docker.sock:/var/run/docker.sock \
    -v ch11-trivy-cache:/root/.cache/trivy \
    aquasec/trivy:0.76.1 image --scanners vuln \
    --severity HIGH,CRITICAL --skip-db-update --exit-code 1 shorty-worker:0.1 \
    >/dev/null 2>&1; echo "trivy exit=$?"
