# syntax=docker/dockerfile:1
# 故障演示：用 ARG 传 token
FROM alpine:3.22
ARG TOKEN
RUN echo "$TOKEN" > /token.txt
CMD ["cat", "/token.txt"]
