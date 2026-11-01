# 14.4：网络、端口和 secret 都从运行中的项目里看
# 密码文件是 secrets/db_password.txt，只在 lab 里使用。
# 服务名就是同一网络里的 DNS 名
docker compose exec -T postgres getent hosts api

# 默认网络按项目命名，五个服务都在里面
docker network ls --filter name=shorty --format '{{.Name}} {{.Driver}}'
docker network inspect shorty_default --format '{{range .Containers}}{{.Name}} {{end}}' | tr ' ' '\n' | sort | tr '\n' ' '

# api 发布到宿主机；postgres 没有 ports，只在网络内可达
docker compose port api 8080
docker compose config --format json | jq '.services.postgres | {ports, expose}'

# secret 在 config 里只是一个文件路径，没有内容
docker compose config --format json | jq '.secrets'
if docker compose config | grep -q 'shorty-demo-password-not-for-production'; then
  echo "config 中出现了明文密码"
else
  echo "config 中没有明文密码"
fi

# api 拿到的是密码文件路径；容器里的 secret 是只读文件
docker compose ps -q api | xargs docker inspect --format '{{range .Config.Env}}{{println .}}{{end}}' | grep DB_PASSWORD
docker compose exec -T postgres stat -c '%a %U %n' /run/secrets/db_password
docker compose exec -T postgres sh -c 'PGPASSWORD=$(cat /run/secrets/db_password) psql -U shorty -d shorty -tAc "select current_user"'
