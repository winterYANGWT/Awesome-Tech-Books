# syntax=docker/dockerfile:1
FROM golang:1.27.3-trixie AS build
WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .
RUN CGO_ENABLED=0 go build -trimpath -o /out/shorty .

FROM scratch
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --from=build /out/shorty /shorty
USER 65532:65532
ENTRYPOINT ["/shorty"]
CMD ["serve"]
