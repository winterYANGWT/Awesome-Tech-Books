FROM alpine:3.22
RUN dd if=/dev/urandom of=/bigfile bs=1M count=10
