# Observability Quick Start

## 1. Start the engine

From the repository root:

```bash
./build/crypto_signal_engine --live
```

The engine exposes metrics at `http://127.0.0.1:9100/metrics`.

## 2. Check the endpoint directly

```bash
curl http://127.0.0.1:9100/metrics
```

Useful checks:

```bash
curl -s http://127.0.0.1:9100/metrics | grep crypto_queue
curl -s http://127.0.0.1:9100/metrics | grep crypto_sequence
curl -s http://127.0.0.1:9100/metrics | grep crypto_book_update_seconds
```

## 3. Start Prometheus

Use the repository configuration:

```bash
prometheus --config.file=deploy/prometheus.yml
```

Open `http://127.0.0.1:9090/targets` and confirm the `crypto_signal_engine` target is `UP`.

If Prometheus runs in Docker and the engine runs directly on the host, change the target in `deploy/prometheus.yml` to `host.docker.internal:9100` and start the container with:

```bash
docker run --add-host=host.docker.internal:host-gateway \
  -p 9090:9090 \
  -v "$PWD/deploy/prometheus.yml:/etc/prometheus/prometheus.yml" \
  prom/prometheus
```

## 4. Import Grafana dashboard

1. Start Grafana and open its web interface.
2. Add a Prometheus data source pointing to `http://localhost:9090`.
3. Use **Dashboards -> Import**.
4. Upload [crypto-signal-overview.json](../dashboards/crypto-signal-overview.json).
5. Map `${DS_PROMETHEUS}` to the Prometheus data source.
6. Select **Last 15 minutes** or another recent range.

## What to watch

- Queue depth and high-water mark show whether processing is falling behind.
- Queue wait p95 shows waiting pressure before book processing.
- Book update p95 shows local order-book work.
- Sequence gaps and recoveries show feed synchronization problems.
- Feed message counts show whether each symbol is active.
- Strategy samples and signals show progress through the rolling warm-up and signal state machine.

This dashboard is intended for diagnosing the educational system's behavior and latency. It is not an operational trading monitor.
