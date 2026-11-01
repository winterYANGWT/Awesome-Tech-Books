# syntax=docker/dockerfile:1

# psql 来自基础镜像；redis 包提供 redis-cli。
FROM postgres:18.6-alpine
RUN apk add --no-cache redis
CMD ["sleep", "infinity"]
