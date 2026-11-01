# 12.2 Docker Hub 与登录
# 这里的 registry 还没配认证，登录照样会成功：CLI 只是把凭据写进本地 config。
printf '%s' s3cret | docker login localhost:5000 -u shorty --password-stdin

jq '{auths: (.auths | keys), credsStore: (.credsStore // null)}' ~/.docker/config.json

#> expect: ^shorty:s3cret$
jq -r '.auths["localhost:5000"].auth' ~/.docker/config.json | base64 -d; echo

if grep -q 's3cret' ~/.docker/config.json; then echo 'config.json 里有明文密码'; else echo 'config.json 里没有明文密码，只有 base64'; fi

docker logout localhost:5000

jq '.auths' ~/.docker/config.json
