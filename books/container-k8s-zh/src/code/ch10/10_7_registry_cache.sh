# 10.6 续：把构建缓存导出到 registry，再清空本地缓存后导回来
# 第一次构建，同时把 cache 推到 registry
#> tail: 6
#> timeout: 300
#> expect: sending cache export
docker buildx build --provenance=false --progress=plain \
    --cache-to type=registry,ref=localhost:5000/shorty:buildcache,mode=max \
    -f shorty.Dockerfile -t shorty:0.1-cache --load ../app/api

# 清掉 builder 的本地缓存，相当于换一台机器
#> tail: 3
docker buildx prune -af --builder ch10
docker buildx du --builder ch10 | tail -4

# 本地没有缓存，但可以从 registry 导回来；只留下导入和命中的行
#> timeout: 300
#> expect: importing cache manifest from localhost:5000/shorty:buildcache
#> expect: CACHED
docker buildx build --provenance=false --progress=plain \
    --cache-from type=registry,ref=localhost:5000/shorty:buildcache \
    -f shorty.Dockerfile -t shorty:0.1-cache --load ../app/api 2>&1 \
    | grep -E "importing cache manifest|CACHED|sending tarball"
