# 默认 bridge 上加 --link：Docker 仍接受，但 29.6 起会打印弃用告警。
#> expect: deprecated
docker run -d --name ch06-link --link shorty-pg:db busybox:1.37 sleep 600 >/dev/null

# link 只做两件事：往接收方的 /etc/hosts 写静态条目，以及（旧版本里）注入环境变量。
docker exec ch06-link cat /etc/hosts
docker exec ch06-link sh -c 'env | grep "^DB_" || echo "没有 DB_* 环境变量"'
docker exec ch06-link ping -c 1 -W 2 db
docker inspect -f '{{json .HostConfig.Links}}' ch06-link

docker rm -f ch06-link >/dev/null
