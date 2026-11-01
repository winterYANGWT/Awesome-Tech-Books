# 9.6：元数据指令与运行时行为
rm -rf /tmp/directive-demo
mkdir -p /tmp/directive-demo/payload
printf 'hello from ADD\n' > /tmp/directive-demo/payload/hello.txt
tar -czf /tmp/directive-demo/payload.tar.gz -C /tmp/directive-demo/payload .

#> tail: 18
docker build --progress=plain --provenance=false --no-cache -f directives.Dockerfile -t shorty:directives /tmp/directive-demo

# ENV 进镜像，ARG 不进
docker image inspect --format '{{json .Config.Env}}' shorty:directives

# EXPOSE、VOLUME、USER、STOPSIGNAL、HEALTHCHECK 都是配置项
docker image inspect --format 'User={{.Config.User}} StopSignal={{.Config.StopSignal}}' shorty:directives
docker image inspect --format '{{json .Config.ExposedPorts}} {{json .Config.Volumes}}' shorty:directives
docker image inspect --format '{{json .Config.Healthcheck}}' shorty:directives

# ADD 自动解开 tar，/opt 里只有解压后的文件
docker run --rm --entrypoint ls shorty:directives -l /opt

# VOLUME 让运行时创建一个匿名卷
docker run -d --name directives-demo shorty:directives
docker inspect --format '{{range .Mounts}}{{.Type}} {{.Destination}} {{.Name}}{{println}}{{end}}' directives-demo
docker rm -fv directives-demo
