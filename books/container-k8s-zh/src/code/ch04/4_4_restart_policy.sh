# 先让 events 订阅建立起来，再启动容器；否则可能错过 create/start。
docker events --since "$(date +%s)" --until "$(date -d '+10 seconds' +%s)" \
  --format '{{.Action}} {{.Actor.Attributes.name}} {{.TimeNano}}' > /tmp/ch04-restart-events.log &
EV=$!
sleep 0.5

# 让容器立即以 1 退出，on-failure:3 会重启 3 次后放弃。
#> hide
docker run -d --name shorty-restart --restart=on-failure:3 busybox:1.37 \
  sh -c 'echo start; exit 1' > /dev/null
for _ in $(seq 1 100); do
  [ "$(docker inspect -f '{{.RestartCount}} {{.State.Status}}' shorty-restart)" = '3 exited' ] && break
  sleep 0.1
done

# 等 events 到 --until 自己退出，确保缓冲写到文件。
#> hide
wait $EV || true

#> expect: restarts=3 exit=1 policy=\{"Name":"on-failure","MaximumRetryCount":3\}
docker inspect -f 'restarts={{.RestartCount}} exit={{.State.ExitCode}} policy={{json .HostConfig.RestartPolicy}}' shorty-restart
#> expect: ^4$
docker logs shorty-restart | grep -c '^start$'

# 每次策略重启都会留下一对 die/start，name 是容器名。
#> expect: ^start shorty-restart
grep -E '^(die|start) shorty-restart ' /tmp/ch04-restart-events.log
# start 之间的间隔逐次变长；具体封顶值官方文档没有写，见来源记录。
#> expect: [0-9]+ ms
grep '^start shorty-restart ' /tmp/ch04-restart-events.log \
  | awk 'NR==1{prev=$3; next}{printf "%d ms\n", ($3-prev)/1000000; prev=$3}'

# 手动 docker restart 的事件窗口里能看到重新 start；当前版本没有稳定的独立 restart 动作。
docker run -d --name shorty-manual busybox:1.37 sleep 1000 > /dev/null
docker events --since "$(date +%s)" --until "$(date -d '+5 seconds' +%s)" \
  --format '{{.Action}} {{.Actor.Attributes.name}}' > /tmp/ch04-manual-events.log &
EV=$!
sleep 0.5
docker restart shorty-manual > /dev/null
docker kill shorty-manual > /dev/null
wait $EV || true
#> expect: ^start shorty-manual$
grep '^start shorty-manual$' /tmp/ch04-manual-events.log

docker rm -f shorty-restart shorty-manual > /dev/null
