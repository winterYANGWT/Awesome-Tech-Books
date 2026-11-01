# 第 4 章的 exec 形式镜像：在 shorty:dev 里编出 /shorty，PID 1 直接是 shorty。
# 保留 shell 和 Go 工具链，便于 docker exec 调试。
FROM shorty:dev
RUN CGO_ENABLED=0 go build -trimpath -o /shorty .
ENTRYPOINT ["/shorty"]
CMD ["serve"]
