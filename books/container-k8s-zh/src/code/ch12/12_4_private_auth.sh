# 12.4 私有仓库认证与 push/pull
# htpasswd -B 生成 bcrypt 哈希；文件里是“用户名:哈希”，不是明文。
mkdir -p /tmp/ch12-auth
docker run --rm httpd:2.4-alpine htpasswd -Bbn shorty s3cret > /tmp/ch12-auth/htpasswd

cut -d: -f1 /tmp/ch12-auth/htpasswd

#> expect: ^\$2y\$$
cut -d: -f2 /tmp/ch12-auth/htpasswd | cut -c1-4

# 用同一份数据卷重建 registry，加上 htpasswd 认证。
docker rm -f shorty-registry >/dev/null 2>&1 || true
docker run -d --name shorty-registry -p 5000:5000 \
  -v registry-data:/var/lib/registry \
  -v /tmp/ch12-auth:/auth \
  -e REGISTRY_AUTH=htpasswd \
  -e REGISTRY_AUTH_HTPASSWD_REALM=shorty \
  -e REGISTRY_AUTH_HTPASSWD_PATH=/auth/htpasswd \
  registry:3.0.0 >/dev/null
for _ in $(seq 1 30); do curl -fsS -u shorty:s3cret localhost:5000/v2/ >/dev/null 2>&1 && break; sleep 0.5; done
curl -fsS -u shorty:s3cret localhost:5000/v2/ >/dev/null

curl -s -D /tmp/ch12-headers -o /dev/null localhost:5000/v2/

#> expect: 401 Unauthorized
tr -d '\r' < /tmp/ch12-headers | grep -iE '^HTTP|^Www-Authenticate'

docker tag shorty:0.1 localhost:5000/shorty:private

# 没有登录时，Docker 拿不到凭据，push 被拒绝。
#> fails
#> expect: no basic auth credentials
docker push localhost:5000/shorty:private 2>&1 | tail -2

printf '%s' s3cret | docker login localhost:5000 -u shorty --password-stdin

docker push localhost:5000/shorty:private 2>&1 | tail -2

# 删掉本地 tag，强制从 registry 拉一次，确认读也需要认证。
docker image rm localhost:5000/shorty:private

#> expect: Status: Downloaded newer image
docker pull localhost:5000/shorty:private 2>&1 | tail -2

docker logout localhost:5000
