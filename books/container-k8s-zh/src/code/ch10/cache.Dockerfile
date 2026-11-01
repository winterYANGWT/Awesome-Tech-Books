# syntax=docker/dockerfile:1

FROM --platform=$BUILDPLATFORM golang:1.27.3-trixie AS build
WORKDIR /src

COPY go.mod go.sum ./

# CACHE_BUST 只用来让下面这层失效，模拟“源码或参数变了，但依赖没变”
ARG CACHE_BUST=0
RUN echo "cache bust: ${CACHE_BUST}"

# id= 给缓存命名；两个 mount 分别存模块和编译结果
RUN --mount=type=cache,id=ch10-gomod,target=/go/pkg/mod \
    --mount=type=cache,id=ch10-gobuild,target=/root/.cache/go-build \
    echo "cached module zips before: $(find /go/pkg/mod/cache/download -name '*.zip' 2>/dev/null | wc -l)" && \
    go mod download && \
    echo "cached module zips after:  $(find /go/pkg/mod/cache/download -name '*.zip' 2>/dev/null | wc -l)"

COPY . .
RUN --mount=type=cache,id=ch10-gomod,target=/go/pkg/mod \
    --mount=type=cache,id=ch10-gobuild,target=/root/.cache/go-build \
    CGO_ENABLED=0 go build -trimpath -ldflags='-s -w' -o /out/shorty .

FROM scratch AS runtime
COPY --from=build /out/shorty /shorty
ENTRYPOINT ["/shorty"]
