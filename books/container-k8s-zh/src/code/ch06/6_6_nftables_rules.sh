# 先确认 daemon 用的是哪个防火墙后端。这个 lab 是 iptables，但 iptables 本身跑在 nf_tables 上。
docker info --format 'firewall_backend={{.FirewallBackend.Driver}}'

# nft 看到的表和 Docker 自己的链；DOCKER 链里是端口发布的 DNAT。
nft list tables
#> expect: DNAT
nft list chain ip nat DOCKER
nft list chain ip nat POSTROUTING

# 整份 ruleset 很长，只按表、链和 xt 兼容目标看结构。
nft list ruleset | grep -E '^(table|[[:space:]]+chain)|xt target' | sed -n '1,24p'

# DOCKER-USER 是留给用户的过滤入口，本 lab 没有额外规则，链是空的。
nft list chain ip filter DOCKER-USER

# 同一批规则从 iptables 侧看，头部会注明 nf_tables，说明不是旧的 x_tables 实现。
iptables-save | sed -n '1p'
iptables-save -t nat | grep -E 'DNAT|MASQUERADE' | sed -n '1,5p'
