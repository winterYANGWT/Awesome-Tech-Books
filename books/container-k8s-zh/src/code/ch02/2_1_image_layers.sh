# config 里的 diffID 是每一层解压后的 sha256；shorty:0.1 有 14 层。
#> expect: "count": 14
docker image inspect shorty:0.1 --format '{{json .RootFS}}' \
  | jq '{type: .Type, count: (.Layers | length), first: .Layers[0], last: .Layers[-1]}'

# history 把层对应回 Dockerfile / 基础镜像的构建指令。
#> expect: COPY /out/shorty /shorty # buildkit
docker history --no-trunc shorty:0.1

# 把镜像导出成 OCI layout，直接看 index、manifest 和 blob。
docker save shorty:0.1 -o /tmp/shorty-0.1.tar
tar -tf /tmp/shorty-0.1.tar > /tmp/shorty-0.1.list
head -12 /tmp/shorty-0.1.list

# index.json 指向多架构 index；这里有一个 amd64 manifest 和一个 attestation。
#> expect: "architecture": "amd64"
(
INDEX=$(tar -xOf /tmp/shorty-0.1.tar index.json | jq -r '.manifests[0].digest')
tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${INDEX#sha256:}" \
  | jq '{mediaType, count: (.manifests | length), manifests: [.manifests[] | {platform, digest}]}'
)

# manifest 才是“config + 若干 layer”的三段结构；layer digest 是压缩后内容的 sha256。
#> expect: "layer_count": 14
(
INDEX=$(tar -xOf /tmp/shorty-0.1.tar index.json | jq -r '.manifests[0].digest')
MANIFEST=$(tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${INDEX#sha256:}" | jq -r '.manifests[0].digest')
tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${MANIFEST#sha256:}" \
  | jq '{config: .config.digest, layer_count: (.layers | length), first_layer: .layers[0], last_layer: .layers[-1]}'
)

# config 里同时有 diffID、history、architecture/os。
#> expect: ENTRYPOINT
(
INDEX=$(tar -xOf /tmp/shorty-0.1.tar index.json | jq -r '.manifests[0].digest')
MANIFEST=$(tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${INDEX#sha256:}" | jq -r '.manifests[0].digest')
CONFIG=$(tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${MANIFEST#sha256:}" | jq -r '.config.digest')
tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${CONFIG#sha256:}" \
  | jq '{architecture, os, layer_count: (.rootfs.diff_ids | length), history_count: (.history | length), metadata_only: [.history[] | select(.empty_layer) | .created_by]}'
)

# 同一个 blob：压缩文件的 sha256 是 layer digest，解压后 tar 的 sha256 是 diffID。
(
INDEX=$(tar -xOf /tmp/shorty-0.1.tar index.json | jq -r '.manifests[0].digest')
MANIFEST=$(tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${INDEX#sha256:}" | jq -r '.manifests[0].digest')
LAYER=$(tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${MANIFEST#sha256:}" | jq -r '.layers[0].digest')
printf '压缩层 sha256: '; tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${LAYER#sha256:}" | sha256sum
printf '解压后 sha256: '; tar -xOf /tmp/shorty-0.1.tar "blobs/sha256/${LAYER#sha256:}" | gzip -dc | sha256sum
)
