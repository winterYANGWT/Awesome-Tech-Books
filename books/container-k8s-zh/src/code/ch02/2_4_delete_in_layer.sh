# 第一层放入一个 10 MB 的随机文件。
docker build -q -t ch02-big:keep -f bigfile.Dockerfile . >/dev/null

# 第二层把它删掉；构建上下文只有 Dockerfile，不会把大文件带进来。
docker build -q -t ch02-big:delete -f rmbig.Dockerfile . >/dev/null

# 删除后的镜像并没有小 10 MB，反而多了一个记录 whiteout 的小层。
#> expect: ch02-big:delete
docker image ls ch02-big

# history 里仍然能看到创建大文件的那一层。
#> expect: 10.5MB
docker history --no-trunc ch02-big:delete

# 三层都在：alpine 基础层、创建大文件的层、删除层。
#> expect: layers=3
docker image inspect ch02-big:delete --format 'layers={{len .RootFS.Layers}}'

# 在容器的合并视图里文件确实没了，但 lower 层的数据还在。
#> fails
#> expect: No such file or directory
docker run --rm ch02-big:delete ls -l /bigfile
