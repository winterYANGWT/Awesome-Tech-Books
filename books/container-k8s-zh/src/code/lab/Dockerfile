# The book's lab: an isolated Docker daemon (dind) with kind, kubectl and helm.
# Every example runs in here, never on the host daemon. Built and started by scripts/lab.sh.
FROM docker@sha256:e486f5f730f413505b97a713fbeccb7ab327a3b259474c7c1155e0c3915c27fe

ARG KIND_VERSION=v0.33.0
ARG KUBECTL_VERSION=v1.37.0
ARG HELM_VERSION=v4.3.1

RUN apk add --no-cache bash coreutils curl findutils git grep iproute2 jq nftables \
        procps sed util-linux util-linux-misc yq \
 && curl -fsSLo /usr/local/bin/kind https://github.com/kubernetes-sigs/kind/releases/download/${KIND_VERSION}/kind-linux-amd64 \
 && curl -fsSLo /usr/local/bin/kubectl https://dl.k8s.io/release/${KUBECTL_VERSION}/bin/linux/amd64/kubectl \
 && echo "$(curl -fsSL https://dl.k8s.io/release/${KUBECTL_VERSION}/bin/linux/amd64/kubectl.sha256)  /usr/local/bin/kubectl" | sha256sum -c - \
 && curl -fsSL https://get.helm.sh/helm-${HELM_VERSION}-linux-amd64.tar.gz | tar -xz -C /usr/local/bin --strip-components=1 linux-amd64/helm \
 && chmod +x /usr/local/bin/kind /usr/local/bin/kubectl

# Docker Hub limits anonymous pulls, and every chapter run starts from a reset lab; pull Docker Hub
# images through Google's mirror of it. (Examples still name images the usual way.)
COPY daemon.json /etc/docker/daemon.json

# Node image for every cluster in the book (Kubernetes 1.37).
ENV KIND_NODE_IMAGE=kindest/node:v1.37.0@sha256:a1ed56cfb0e7b93589bdf97c8cd566405a265939e3620fc4f5de89adff580ae5
WORKDIR /book
