# 一次构建 api 和 worker；两个 target 并行执行
group "default" {
  targets = ["api", "worker"]
}

target "api" {
  context    = "../app/api"
  dockerfile = "../../ch10/shorty.Dockerfile"
  tags       = ["shorty:0.1"]
  args = {
    VERSION = "0.1"
  }
}

target "worker" {
  context    = "../app/worker"
  dockerfile = "../../ch10/worker.Dockerfile"
  tags       = ["shorty-worker:0.1"]
}
