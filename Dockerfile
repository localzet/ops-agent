FROM debian:bookworm-slim AS builder

RUN apt-get -o Acquire::Retries=5 update \
    && apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        libcurl4-openssl-dev \
        libsqlite3-dev \
        ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPS_AGENT_BUILD_TESTS=OFF \
    && cmake --build build --target ops-agent

FROM debian:bookworm-slim AS runtime

RUN apt-get -o Acquire::Retries=5 update \
    && apt-get -o Acquire::Retries=5 install -y --no-install-recommends \
        ca-certificates \
        curl \
        libcurl4 \
        libsqlite3-0 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --create-home --home-dir /var/lib/ops-agent --shell /usr/sbin/nologin ops-agent

COPY --from=builder /src/build/ops-agent /usr/local/bin/ops-agent
COPY config/config.example.yml /etc/ops-agent/config.yml

USER ops-agent
EXPOSE 8080

ENV OPS_AGENT_CONFIG=/etc/ops-agent/config.yml
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD curl -fsS http://127.0.0.1:8080/health || exit 1

ENTRYPOINT ["/usr/local/bin/ops-agent"]
