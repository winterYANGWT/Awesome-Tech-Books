# 9.2：同一个目录，加不加 .dockerignore，发给 builder 的字节数差多少
rm -rf /tmp/ctx-raw /tmp/ctx-slim
mkdir -p /tmp/ctx-raw/.git/objects/pack /tmp/ctx-raw/testdata /tmp/ctx-raw/node_modules
printf 'shorty\n' > /tmp/ctx-raw/app.txt
dd if=/dev/zero of=/tmp/ctx-raw/.git/objects/pack/pack.bin bs=1M count=12 status=none
dd if=/dev/zero of=/tmp/ctx-raw/testdata/links.json bs=1M count=6 status=none
dd if=/dev/zero of=/tmp/ctx-raw/node_modules/blob.bin bs=1M count=4 status=none
cp -a /tmp/ctx-raw /tmp/ctx-slim

cat > /tmp/ctx-raw/Dockerfile <<'EOF'
FROM scratch
COPY . /ctx/
EOF

cat > /tmp/ctx-slim/Dockerfile <<'EOF'
FROM scratch
COPY . /ctx/
EOF

cat > /tmp/ctx-slim/.dockerignore <<'EOF'
.git
node_modules
testdata
EOF

# 没有 .dockerignore：.git、测试数据和 node_modules 都被打包发送
#> expect: transferring context: 2[0-9]\.[0-9]+MB
# COPY . . 让所有文件都进入构建，才能看出 .dockerignore 的影响
docker build --progress=plain --provenance=false --no-cache -t ctx:raw /tmp/ctx-raw

# 加上 .dockerignore 后，同一个目录只剩应用文件
#> expect: transferring context: [0-9]+B
docker build --progress=plain --provenance=false --no-cache -t ctx:slim /tmp/ctx-slim
