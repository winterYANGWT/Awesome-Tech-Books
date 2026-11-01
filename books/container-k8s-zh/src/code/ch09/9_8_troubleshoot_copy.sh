# 9.8：go.sum 被 .dockerignore 排除，COPY 找不到文件
rm -rf /tmp/shorty-copyfail
cp -a ../app/api /tmp/shorty-copyfail
printf 'go.sum\n' >> /tmp/shorty-copyfail/.dockerignore

# 文件确实在上下文目录里
ls -l /tmp/shorty-copyfail/go.sum

#> fails
#> expect: failed to compute cache key|not found|excluded
docker build --progress=plain --provenance=false -f first.Dockerfile -t shorty:copyfail /tmp/shorty-copyfail

# 从 .dockerignore 里删掉 go.sum 这一行
sed -i '/^go\.sum$/d' /tmp/shorty-copyfail/.dockerignore

#> tail: 6
docker build --progress=plain --provenance=false -f first.Dockerfile -t shorty:copyfail /tmp/shorty-copyfail
