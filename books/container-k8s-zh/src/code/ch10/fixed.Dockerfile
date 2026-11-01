# syntax=docker/dockerfile:1
# 修复：secret 只在 RUN 期间存在，不写进文件系统
FROM alpine:3.22
RUN --mount=type=secret,id=git_token,target=/run/secrets/git_token \
    TOKEN="$(cat /run/secrets/git_token)" && \
    test -n "$TOKEN" && echo "secret read, not written" > /status.txt
CMD ["cat", "/status.txt"]
