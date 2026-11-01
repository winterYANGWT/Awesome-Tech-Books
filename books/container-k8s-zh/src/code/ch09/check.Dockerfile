# syntax=docker/dockerfile:1
FROM ubuntu:latest
ENV TOKEN=shorty-demo-token
ARG PASSWORD=hunter2
MAINTAINER someone@example.com
RUN apt-get update && apt-get install -y curl
CMD ["bash"]
