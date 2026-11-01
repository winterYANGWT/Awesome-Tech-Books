# 9.7：只做静态检查，不执行构建
#> fails
#> expect: SecretsUsedInArgOrEnv
#> expect: MaintainerDeprecated
docker build --check -f check.Dockerfile .
