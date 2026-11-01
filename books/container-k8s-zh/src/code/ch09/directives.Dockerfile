# syntax=docker/dockerfile:1
FROM alpine:3.22

ARG BUILD_ONLY=from-arg
ENV PERSISTED=from-env

EXPOSE 8080
VOLUME /data
STOPSIGNAL SIGQUIT
HEALTHCHECK --interval=5s --timeout=2s --retries=2 --start-period=3s \
  CMD wget -qO- http://127.0.0.1:8080/healthz || exit 1

USER 65532:65532
RUN id
ADD payload.tar.gz /opt/

CMD ["sleep", "300"]
