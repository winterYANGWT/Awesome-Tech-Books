# syntax=docker/dockerfile:1
FROM alpine:3.22
ARG BUILD_TOKEN
RUN test -n "$BUILD_TOKEN" && echo "构建期参数可见"
CMD ["true"]
