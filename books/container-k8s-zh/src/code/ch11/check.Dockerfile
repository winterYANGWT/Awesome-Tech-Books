# syntax=docker/dockerfile:1
# check=error=true

# 四个问题放在一起：未固定的基础镜像、apt 缓存没清、远程 ADD、用 ARG/ENV 传密码，
# 再加一个 shell 形式的 CMD。--check 在这个版本只报后两个。
FROM debian:latest
ARG DB_PASSWORD
ENV DB_PASSWORD=${DB_PASSWORD}
RUN apt-get update && apt-get install -y curl
ADD https://example.com/shorty.tar.gz /opt/
CMD echo "shorty"
