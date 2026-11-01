# 10.3：第一次把依赖下载进 cache mount，第二次换一个 CACHE_BUST 让 RUN 重跑
#> tail: 26
#> expect: cached module zips before: 0
#> timeout: 300
docker buildx build --provenance=false --progress=plain --build-arg CACHE_BUST=1 \
    -f cache.Dockerfile -t shorty:cachetest --load ../app/api

#> tail: 24
#> expect: cached module zips before: [1-9][0-9]*
docker buildx build --provenance=false --progress=plain --build-arg CACHE_BUST=2 \
    -f cache.Dockerfile -t shorty:cachetest --load ../app/api

# --no-cache 连 cache mount 也不复用，依赖会重新下载
#> tail: 24
#> expect: cached module zips before: 0
docker buildx build --provenance=false --progress=plain --no-cache --build-arg CACHE_BUST=3 \
    -f cache.Dockerfile -t shorty:cachetest --load ../app/api

# 依赖缓存留在 builder 里，不在镜像的层里
docker image inspect --format 'image layers={{len .RootFS.Layers}}' shorty:cachetest

#> tail: 8
docker buildx du --builder ch10

# --no-cache 没有删掉磁盘上的缓存；下一次正常构建仍然命中
#> tail: 24
#> expect: cached module zips before: [1-9][0-9]*
docker buildx build --provenance=false --progress=plain --build-arg CACHE_BUST=4 \
    -f cache.Dockerfile -t shorty:cachetest --load ../app/api
