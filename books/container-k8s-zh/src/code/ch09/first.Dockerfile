# syntax=docker/dockerfile:1
FROM golang:1.27.3-trixie

WORKDIR /src

COPY go.mod go.sum ./
RUN go mod download

COPY . .
RUN CGO_ENABLED=0 go build -trimpath -o /shorty .

ENTRYPOINT ["/shorty"]
CMD ["serve"]
