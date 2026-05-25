# ops-agent

Lightweight Linux observability and diagnostics microservice built with C++20 and Crow.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Run

```bash
OPS_AGENT_CONFIG=config/config.example.yml ./build/ops-agent
```

Common environment overrides:

```bash
OPS_PORT=8080
OPS_API_KEY=change-me
OPS_LOG_LEVEL=info
```

## Endpoints

- `GET /health`
- `GET /health/live`
- `GET /health/ready`
- `GET /system`
- `GET /services?names=postfix,docker`
- `GET /check/tcp?host=1.1.1.1&port=443`
- `GET /metrics`
