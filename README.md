# ops-agent

Lightweight Linux observability and diagnostics microservice built with C++20 and Crow.

## Features

- `GET /health`, `/health/live`, `/health/ready`
- `GET /system`
- `GET /services?names=postfix,docker`
- `GET /check/tcp?host=1.1.1.1&port=443`
- `GET /metrics`
- API key authentication via `X-API-Key`
- Per-IP token bucket rate limiting
- Structured JSON request logs
- Docker and systemd deployment

## Requirements

Ubuntu/Debian build dependencies:

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build git \
  libasio-dev libcurl4-openssl-dev libsqlite3-dev
```

Runtime dependencies:

```bash
sudo apt-get install -y --no-install-recommends \
  ca-certificates curl libcurl4 libsqlite3-0
```

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target ops-agent
```

Or use Make:

```bash
make configure
make build
make test
```

## Local Run

```bash
export OPS_AGENT_CONFIG="$PWD/config/config.example.yml"
export OPS_API_KEY="change-me"
./build/ops-agent
```

The default bind address is `127.0.0.1`. Keep it local unless the service is protected by a trusted network boundary or reverse proxy.

## Configuration

Main config file:

```bash
sudo install -d -o root -g root -m 0755 /etc/ops-agent
sudo install -m 0640 config/config.example.yml /etc/ops-agent/config.yml
```

Important YAML settings:

```yaml
server:
  host: "127.0.0.1"
  port: 8080
  threads: 4
  timeout_sec: 5
  max_request_body_bytes: 65536

diagnostics:
  tcp_timeout_ms: 2000
  systemctl_timeout_ms: 2000
  max_services_per_request: 20

security:
  api_key: ""

rate_limit:
  enabled: true
  requests_per_second: 10
  burst: 20
```

Production ENV overrides:

```bash
OPS_PORT=8080
OPS_API_KEY=change-me
OPS_LOG_LEVEL=info
```

Supported additional ENV:

```bash
OPS_AGENT_CONFIG=/etc/ops-agent/config.yml
OPS_AGENT_HOST=127.0.0.1
OPS_AGENT_THREADS=4
OPS_AGENT_TIMEOUT_SEC=5
OPS_AGENT_MAX_REQUEST_BODY_BYTES=65536
OPS_AGENT_TCP_TIMEOUT_MS=2000
OPS_AGENT_SYSTEMCTL_TIMEOUT_MS=2000
OPS_AGENT_MAX_SERVICES_PER_REQUEST=20
OPS_AGENT_SQLITE_PATH=/var/lib/ops-agent/ops-agent.db
OPS_AGENT_RATE_LIMIT_ENABLED=true
OPS_AGENT_RATE_LIMIT_RPS=10
OPS_AGENT_RATE_LIMIT_BURST=20
```

## systemd Install

Build and install:

```bash
sudo useradd --system --home-dir /var/lib/ops-agent --create-home --shell /usr/sbin/nologin ops-agent || true
sudo install -d -o ops-agent -g ops-agent -m 0750 /var/lib/ops-agent
sudo install -d -o root -g root -m 0755 /etc/ops-agent

sudo install -m 0755 build/ops-agent /usr/local/bin/ops-agent
sudo install -m 0640 config/config.example.yml /etc/ops-agent/config.yml
sudo install -m 0640 packaging/systemd/ops-agent.env /etc/ops-agent/ops-agent.env
sudo install -m 0644 packaging/systemd/ops-agent.service /etc/systemd/system/ops-agent.service

sudo sed -i 's/change-me-generate-a-long-random-key/'"$(openssl rand -hex 32)"'/' /etc/ops-agent/ops-agent.env

sudo systemctl daemon-reload
sudo systemctl enable --now ops-agent
sudo systemctl status ops-agent --no-pager
```

Logs:

```bash
journalctl -u ops-agent -f
```

Upgrade binary:

```bash
cmake --build build --target ops-agent
sudo install -m 0755 build/ops-agent /usr/local/bin/ops-agent
sudo systemctl restart ops-agent
```

## Docker

Build:

```bash
docker build -t ops-agent:latest .
```

Run locally bound to loopback:

```bash
docker run --rm \
  --name ops-agent \
  -p 127.0.0.1:8080:8080 \
  -e OPS_AGENT_HOST=0.0.0.0 \
  -e OPS_API_KEY="$(openssl rand -hex 32)" \
  ops-agent:latest
```

Docker Compose:

```bash
export OPS_API_KEY="$(openssl rand -hex 32)"
docker compose up -d --build
docker compose logs -f ops-agent
```

## curl Examples

Set variables:

```bash
export OPS_AGENT_URL="http://127.0.0.1:8080"
export OPS_API_KEY="change-me"
```

Health:

```bash
curl -fsS "$OPS_AGENT_URL/health/live"
curl -fsS "$OPS_AGENT_URL/health/ready"
```

Authenticated endpoints:

```bash
curl -fsS -H "X-API-Key: $OPS_API_KEY" "$OPS_AGENT_URL/system" | jq
curl -fsS -H "X-API-Key: $OPS_API_KEY" "$OPS_AGENT_URL/services?names=ssh,docker,postgresql" | jq
curl -fsS -H "X-API-Key: $OPS_API_KEY" "$OPS_AGENT_URL/check/tcp?host=1.1.1.1&port=443" | jq
curl -fsS -H "X-API-Key: $OPS_API_KEY" "$OPS_AGENT_URL/metrics"
```

Request ID:

```bash
curl -i -H "X-API-Key: $OPS_API_KEY" -H "X-Request-Id: deploy-check-1" "$OPS_AGENT_URL/system"
```

## Healthcheck Examples

Docker:

```bash
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
  CMD curl -fsS http://127.0.0.1:8080/health/live || exit 1
```

Kubernetes probes:

```yaml
livenessProbe:
  httpGet:
    path: /health/live
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 30
readinessProbe:
  httpGet:
    path: /health/ready
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 10
```

systemd manual check:

```bash
curl -fsS http://127.0.0.1:8080/health/live
```

## nginx Reverse Proxy

Install example:

```bash
sudo install -m 0644 packaging/nginx/ops-agent.conf /etc/nginx/conf.d/ops-agent.conf
sudo nginx -t
sudo systemctl reload nginx
```

Example request through nginx:

```bash
curl -fsS -H "X-API-Key: $OPS_API_KEY" https://ops-agent.example.com/system
```

## Security Notes

- Keep `server.host` as `127.0.0.1` when using nginx, VPN, SSH tunnel, or sidecar access.
- Always set `OPS_API_KEY` for any deployment reachable by other machines.
- Do not expose `/check/tcp` to untrusted clients; it is intentionally a network diagnostic endpoint.
- Prefer TLS termination at nginx or another trusted reverse proxy.
- Store `/etc/ops-agent/ops-agent.env` with mode `0640` or stricter.
