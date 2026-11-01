# syntax=docker/dockerfile:1

# 构建阶段固定在 builder 自己的平台上；多架构构建时用 TARGETOS/TARGETARCH 交叉编译。
FROM --platform=$BUILDPLATFORM golang:1.27.3-trixie AS build
ARG TARGETOS
ARG TARGETARCH
ARG VERSION=0.1
WORKDIR /src

COPY go.mod go.sum ./
RUN --mount=type=cache,id=shorty-gomod,target=/go/pkg/mod \
    go mod download

COPY . .
RUN --mount=type=cache,id=shorty-gomod,target=/go/pkg/mod \
    --mount=type=cache,id=shorty-gobuild,target=/root/.cache/go-build \
    CGO_ENABLED=0 GOOS=${TARGETOS} GOARCH=${TARGETARCH} \
    go build -trimpath -ldflags="-s -w -X main.version=${VERSION}" -o /out/shorty .

# 开发阶段继承构建阶段：有 shell、Go 工具链和源码，用来进容器排障。
FROM build AS dev
WORKDIR /src
CMD ["bash"]

# 运行阶段只拿二进制；scratch 里没有 shell、包管理器和 Go。
FROM scratch AS runtime
COPY --from=build /out/shorty /shorty
USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]
