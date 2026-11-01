# 13.8：构建配置本身也要过门禁；这三类问题不必真的构建就能发现
#> fails
#> expect: MaintainerDeprecated
#> expect: SecretsUsedInArgOrEnv
#> expect: JSONArgsRecommended
docker build --check -f check.Dockerfile .

#> expect: Check complete, no warnings found
docker build --check -f check-fixed.Dockerfile .
