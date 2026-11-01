FROM golang:1.27.3-trixie AS build
WORKDIR /src
COPY main.go .
RUN CGO_ENABLED=0 go build -trimpath -ldflags="-s -w" -o /out/loadgen ./main.go

FROM scratch
COPY --from=build /out/loadgen /loadgen
ENTRYPOINT ["/loadgen"]
