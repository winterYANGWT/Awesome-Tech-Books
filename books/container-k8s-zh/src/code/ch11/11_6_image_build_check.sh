# 11.6：--check 只做静态检查，不执行构建
#> fails
#> expect: JSONArgsRecommended
#> expect: SecretsUsedInArgOrEnv
docker build --check -f check.Dockerfile . 2>&1 | grep -E '^(Check complete|WARNING|ERROR)'

# 同一个文件，check=error=true 让普通构建也在检查阶段停下
#> fails
#> expect: lint violation found
docker build -f check.Dockerfile . 2>&1 | tail -14
