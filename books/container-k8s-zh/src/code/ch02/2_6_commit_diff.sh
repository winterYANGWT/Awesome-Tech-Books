# 一个带 /app 三个文件的基础镜像。
docker build -q -t ch02-commit-base:1 -f commit-base.Dockerfile . >/dev/null
docker run -d --name ch02-commit ch02-commit-base:1 sleep 300

# 在容器里新增、修改、删除各一个文件。
docker exec ch02-commit sh -c 'echo new > /app/added.txt; echo changed >> /app/change.txt; rm /app/delete.txt'

# docker diff 的三个标记：A 新增、C 修改、D 删除。
#> expect: A /app/added.txt
#> expect: C /app/change.txt
#> expect: D /app/delete.txt
docker diff ch02-commit

# commit 把可写层固化成新镜像；history 能看到这条提交。
#> expect: add/change/delete under /app
(
docker commit -m 'add/change/delete under /app' ch02-commit ch02-committed:1
docker history --no-trunc ch02-committed:1
)

# export/import 把文件系统压平成一个层，同时丢掉 CMD/ENTRYPOINT 等元数据。
(
docker export ch02-commit -o /tmp/ch02-commit.tar
docker import /tmp/ch02-commit.tar ch02-flat:1
)

#> expect: layers=1
(
docker history --no-trunc ch02-flat:1
docker image inspect ch02-flat:1 --format 'layers={{len .RootFS.Layers}} cmd={{json .Config.Cmd}} entrypoint={{json .Config.Entrypoint}}'
)

# 清理容器。
docker rm -f ch02-commit
