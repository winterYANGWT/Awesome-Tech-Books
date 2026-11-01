# 9.9：源码被 .dockerignore 排除，改了代码缓存也不会失效
rm -rf /tmp/stale-demo
mkdir -p /tmp/stale-demo
cat > /tmp/stale-demo/go.mod <<'EOF'
module stale

go 1.27.3
EOF
cat > /tmp/stale-demo/main.go <<'EOF'
package main

import "fmt"

var version = "base"

func main() {
	fmt.Println(version)
}
EOF
cat > /tmp/stale-demo/settings.go <<'EOF'
package main

func init() {
	version = "v1"
}
EOF
printf 'settings.go\n' > /tmp/stale-demo/.dockerignore
cat > /tmp/stale-demo/Dockerfile <<'EOF'
FROM golang:1.27.3-trixie
WORKDIR /src
COPY . .
RUN CGO_ENABLED=0 go build -o /stale .
ENTRYPOINT ["/stale"]
EOF

# settings.go 被排除，构建仍然成功，但版本是 base
#> tail: 6
docker build --progress=plain --provenance=false -t stale:v1 /tmp/stale-demo
docker run --rm stale:v1

# 改源码后再构建：COPY 的校验和没变，整段缓存命中
sed -i 's/version = "v1"/version = "v2"/' /tmp/stale-demo/settings.go
#> expect: COPY \. \.\n#\d+ CACHED
docker build --progress=plain --provenance=false -t stale:v2 /tmp/stale-demo
docker run --rm stale:v2

# --no-cache 只是重新执行指令，settings.go 仍然不在上下文里
#> tail: 6
docker build --progress=plain --provenance=false --no-cache -t stale:v2 /tmp/stale-demo
docker run --rm stale:v2

# 修好 .dockerignore，COPY 和 go build 才重新执行
: > /tmp/stale-demo/.dockerignore
#> tail: 16
docker build --progress=plain --provenance=false -t stale:v2 /tmp/stale-demo
docker run --rm stale:v2
