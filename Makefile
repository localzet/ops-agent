.PHONY: configure build test clean run docker-build docker-run compose-up compose-down install-systemd uninstall-systemd

BUILD_DIR ?= build
BUILD_TYPE ?= Release
PORT ?= 8080
API_KEY ?= change-me

configure:
	cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build:
	cmake --build $(BUILD_DIR) --target ops-agent

test:
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	cmake -E rm -rf $(BUILD_DIR)

run:
	OPS_AGENT_CONFIG=config/config.example.yml OPS_PORT=$(PORT) OPS_API_KEY=$(API_KEY) ./$(BUILD_DIR)/ops-agent

docker-build:
	docker build -t ops-agent:latest .

docker-run:
	docker run --rm --name ops-agent -p 127.0.0.1:$(PORT):8080 -e OPS_AGENT_HOST=0.0.0.0 -e OPS_API_KEY=$(API_KEY) ops-agent:latest

compose-up:
	OPS_API_KEY=$(API_KEY) docker compose up -d --build

compose-down:
	docker compose down

install-systemd:
	sudo useradd --system --home-dir /var/lib/ops-agent --create-home --shell /usr/sbin/nologin ops-agent || true
	sudo install -d -o ops-agent -g ops-agent -m 0750 /var/lib/ops-agent
	sudo install -d -o root -g root -m 0755 /etc/ops-agent
	sudo install -m 0755 $(BUILD_DIR)/ops-agent /usr/local/bin/ops-agent
	sudo install -m 0640 config/config.example.yml /etc/ops-agent/config.yml
	sudo install -m 0640 packaging/systemd/ops-agent.env /etc/ops-agent/ops-agent.env
	sudo sed -i 's/change-me-generate-a-long-random-key/$(API_KEY)/' /etc/ops-agent/ops-agent.env
	sudo install -m 0644 packaging/systemd/ops-agent.service /etc/systemd/system/ops-agent.service
	sudo systemctl daemon-reload
	sudo systemctl enable --now ops-agent

uninstall-systemd:
	sudo systemctl disable --now ops-agent || true
	sudo rm -f /etc/systemd/system/ops-agent.service
	sudo systemctl daemon-reload
