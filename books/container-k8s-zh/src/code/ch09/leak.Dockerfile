# syntax=docker/dockerfile:1
FROM alpine:3.22
ARG BUILD_TOKEN=only-during-build
ENV TOKEN=shorty-demo-token
RUN test -n "$BUILD_TOKEN" && echo "构建期参数可见"
CMD ["sh", "-c", "echo \"$TOKEN\"; sleep 300"]
