# syntax=docker/dockerfile:1

FROM --platform=$BUILDPLATFORM golang:1.27.3-trixie AS build
ARG TARGETOS
ARG TARGETARCH
ARG VERSION=0.1
WORKDIR /src

COPY go.mod go.sum ./
RUN --mount=type=cache,id=ch11-gomod,target=/go/pkg/mod go mod download

COPY . .
RUN --mount=type=cache,id=ch11-gomod,target=/go/pkg/mod \
    --mount=type=cache,id=ch11-gobuild,target=/root/.cache/go-build \
    GOMAXPROCS=1 GOGC=20 GOMEMLIMIT=300MiB CGO_ENABLED=0 GOOS=${TARGETOS} GOARCH=${TARGETARCH} \
    go build -p 1 -trimpath -ldflags="-s -w -X main.version=${VERSION}" -o /out/shorty .

FROM scratch AS runtime
ARG VERSION=0.1
ARG REVISION=unknown
ARG CREATED=unknown
ARG SOURCE=https://git.example/shorty

# 这些键来自 OCI 镜像规范。写在 LABEL 里会进入 Config.Labels，docker image inspect 能直接看到；
# 推到 registry 时还可以用 --annotation 把同样的键挂到 manifest/index 上（本章 11.6 讲差别）。
LABEL org.opencontainers.image.title="shorty" \
      org.opencontainers.image.description="shorty URL shortener API" \
      org.opencontainers.image.version="${VERSION}" \
      org.opencontainers.image.revision="${REVISION}" \
      org.opencontainers.image.created="${CREATED}" \
      org.opencontainers.image.source="${SOURCE}" \
      org.opencontainers.image.licenses="Apache-2.0"

COPY --chown=65532:65532 --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --chown=65532:65532 --from=build /usr/share/zoneinfo /usr/share/zoneinfo
COPY --link --chown=65532:65532 --from=build /out/shorty /shorty

USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]
