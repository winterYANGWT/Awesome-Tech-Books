FROM alpine:3.22
RUN mkdir -p /app \
 && echo keep > /app/keep.txt \
 && echo old > /app/change.txt \
 && echo gone > /app/delete.txt
