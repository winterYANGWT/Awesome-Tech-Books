# devcontainer.json 的 build.target 用 api Dockerfile 的 build 阶段（带 Go 工具链），
# 不是生产用的 scratch 阶段。
(
  set -e
  ./devc up --workspace-folder /book/src/code/app/api \
    --config /book/src/code/ch15/devcontainer.json \
    --remove-existing-container \
    --no-lockfile > /tmp/ch15-devcontainer.log 2>&1
  tail -n 3 /tmp/ch15-devcontainer.log
)

./devc exec --workspace-folder /book/src/code/app/api \
  --config /book/src/code/ch15/devcontainer.json go version

# go test 在开发容器里跑，和同事机器上的 Go 版本无关。
#> timeout: 300
./devc exec --workspace-folder /book/src/code/app/api \
  --config /book/src/code/ch15/devcontainer.json go test -p 1 ./...
