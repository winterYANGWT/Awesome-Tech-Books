# 模拟项目里的 api/templates：宿主目录挂进容器，改文件后不用重启容器。
rm -rf /tmp/ch15-bind
mkdir -p /tmp/ch15-bind/api/templates /tmp/ch15-bind/api/data
chmod 777 /tmp/ch15-bind/api/data
cat > /tmp/ch15-bind/api/templates/index.html <<'HTML'
<!doctype html>
<html lang="zh-CN">
  <head><meta charset="utf-8"><title>Shorty dev</title></head>
  <body><h1>Version 1</h1></body>
</html>
HTML
docker run -d --name ch15-bind -p 18080:80 \
  --mount type=bind,source=/tmp/ch15-bind/api/templates,target=/usr/share/nginx/html,readonly \
  nginx:1.29-alpine >/dev/null
for _ in $(seq 1 30); do
  curl -fsS http://localhost:18080/ >/tmp/ch15-page 2>/dev/null && break
  sleep 1
done
grep -o 'Version 1' /tmp/ch15-page

# 改宿主文件，容器里立刻看到；nginx 进程没有重启。
sed -i 's/Version 1/Version 2/' /tmp/ch15-bind/api/templates/index.html
for _ in $(seq 1 30); do
  curl -fsS http://localhost:18080/ 2>/dev/null | grep -q 'Version 2' && break
  sleep 1
done
curl -fsS http://localhost:18080/ | grep -o 'Version 2'
ls -ln /tmp/ch15-bind/api/templates
docker exec ch15-bind ls -ln /usr/share/nginx/html

# 只读挂载挡住容器内的写操作。
#> fails
#> expect: Read-only file system
docker exec ch15-bind sh -c 'echo changed > /usr/share/nginx/html/index.html'

# 换成可写挂载，以 uid 1000 写入，宿主上看到的新文件属主就是 1000。
docker run --rm --user 1000:1000 -v /tmp/ch15-bind/api/data:/data \
  busybox:1.37 sh -c 'echo from-container > /data/note.txt'
ls -ln /tmp/ch15-bind/api/data

docker rm -f ch15-bind >/dev/null
