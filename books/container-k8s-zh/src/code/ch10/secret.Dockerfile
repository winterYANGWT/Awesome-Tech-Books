# syntax=docker/dockerfile:1

FROM --platform=$BUILDPLATFORM golang:1.27.3-trixie AS build
WORKDIR /src

COPY go.mod ./
COPY main.go ./

# token 只在这条 RUN 里以文件形式出现；不会留在层里
RUN --mount=type=secret,id=git_token,target=/run/secrets/git_token \
    --mount=type=cache,id=ch10-secret-gomod,target=/go/pkg/mod \
    TOKEN="$(cat /run/secrets/git_token)" && \
    curl -fsS -H "Authorization: Bearer ${TOKEN}" \
      http://localhost:8080/secretlib.tar.gz -o /tmp/secretlib.tar.gz && \
    mkdir -p /src/secretlib && \
    tar -xzf /tmp/secretlib.tar.gz -C /src && \
    go mod edit -replace example.com/secretlib=/src/secretlib && \
    CGO_ENABLED=0 go build -trimpath -ldflags='-s -w' -o /out/secretapp .

FROM alpine:3.22
COPY --from=build /out/secretapp /secretapp
ENTRYPOINT ["/secretapp"]
