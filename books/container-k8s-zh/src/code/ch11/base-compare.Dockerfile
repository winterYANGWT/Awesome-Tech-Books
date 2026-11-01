# syntax=docker/dockerfile:1

# 同一份二进制分别放进 alpine、python slim、scratch，用来看基础镜像本身带来的体积和文件。
FROM golang:1.27.3-trixie AS build
WORKDIR /src
COPY go.mod go.sum ./
RUN --mount=type=cache,id=ch11-gomod,target=/go/pkg/mod go mod download
COPY . .
RUN --mount=type=cache,id=ch11-gomod,target=/go/pkg/mod \
    --mount=type=cache,id=ch11-gobuild,target=/root/.cache/go-build \
    GOMAXPROCS=1 GOGC=20 GOMEMLIMIT=300MiB CGO_ENABLED=0 GOOS=linux GOARCH=amd64 \
    go build -p 1 -trimpath -ldflags="-s -w -X main.version=0.1" -o /out/shorty .

FROM alpine:3.22 AS alpine
COPY --chown=65532:65532 --from=build /out/shorty /shorty
USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]

FROM python:3.15.2-slim-trixie AS slim
COPY --chown=65532:65532 --from=build /out/shorty /shorty
USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]

# scratch 没有证书和时区，需要从构建阶段自己带；这是它和 distroless 的主要差别之一。
FROM scratch AS minimal
COPY --chown=65532:65532 --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --chown=65532:65532 --from=build /usr/share/zoneinfo /usr/share/zoneinfo
COPY --link --chown=65532:65532 --from=build /out/shorty /shorty
USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]
