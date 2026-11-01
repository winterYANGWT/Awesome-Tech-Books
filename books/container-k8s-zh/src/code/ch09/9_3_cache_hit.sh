# 9.3：先完整构建一次，再原样构建一次，最后只改一行 Go 源码
# lab 的构建缓存跨章节保留，先清空，保证三次构建的结果只取决于本示例
#> hide
docker builder prune -af
rm -rf /tmp/shorty-cache
cp -a ../app/api /tmp/shorty-cache

# 第一次：所有步骤都执行（--no-cache 让 9.1 留下的缓存失效）
#> tail: 18
docker build --progress=plain --provenance=false --no-cache -f first.Dockerfile -t shorty:cache-1 /tmp/shorty-cache

# 第二次：源码没变，BuildKit 直接复用缓存
#> expect: RUN go mod download\n#\d+ CACHED
#> expect: go build[^\n]*\n#\d+ CACHED
docker build --progress=plain --provenance=false -f first.Dockerfile -t shorty:cache-2 /tmp/shorty-cache

# 改 http.go 的注释，并新增一个不参与编译的文件，确保 COPY 的输入集合一定变化
printf '\n// 只改这一行注释\n' >> /tmp/shorty-cache/http.go
printf 'cache experiment\n' > /tmp/shorty-cache/cache-note.txt
touch -m -d '2030-01-01 00:00:00' /tmp/shorty-cache/cache-note.txt
#> tail: 18
#> expect: RUN go mod download\n#\d+ CACHED
#> expect: COPY \. \.\n#\d+ DONE
docker build --progress=plain --provenance=false -f first.Dockerfile -t shorty:cache-3 /tmp/shorty-cache
