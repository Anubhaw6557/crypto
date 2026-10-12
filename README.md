# Live Crypto Signal Engine

A placement-focused C++20 project that consumes live Binance Spot market data, maintains local order books, and generates experimental BTC/ETH mean-reversion signals.

This project does not place orders, perform backtesting, or claim strategy profitability. Signals are written to the server log. Metrics are exposed locally for Prometheus and Grafana.

## Features

- Live Binance depth streams for BTCUSDT, ETHUSDT, SOLUSDT, BNBUSDT, and XRPUSDT
- TLS WebSocket connection with reconnect handling
- REST order-book snapshots and sequence-gap recovery
- Per-symbol local order books with best bid, best ask, mid-price, spread, and freshness
- Online BTC/ETH spread strategy with configurable beta, warm-up, z-score thresholds, and state transitions
- Bounded SPSC queue between feed and processing code
- Prometheus-compatible metrics endpoint
- Grafana dashboard for feed health, queues, order books, latency, recovery, and signals
- CMake, CTest, and deterministic unit tests

## Requirements

- Linux
- C++20 compiler
- CMake 3.20 or newer
- Boost development headers
- OpenSSL development libraries
- libcurl development libraries

On Ubuntu, install the native dependencies with:

```bash
sudo apt update
sudo apt install -y g++ cmake libboost1.90-dev libcurl4-openssl-dev libssl-dev
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The executable is created at `build/crypto_signal_engine`.

## Run Modes

Show command-line help:

```bash
./build/crypto_signal_engine --help
```

Run the local startup smoke mode without connecting to Binance:

```bash
./build/crypto_signal_engine --once
```

Run the live system:

```bash
./build/crypto_signal_engine --live
```

The live mode:

1. Opens the local metrics endpoint on port `9100`.
2. Connects to Binance depth streams.
3. Fetches REST snapshots for the configured symbols.
4. Applies WebSocket depth updates to local order books.
5. Recovers books from REST after detected sequence gaps.
6. Samples BTC/ETH mid-prices every second.
7. Logs strategy signals after the rolling window has warmed up.

Stop the live process with `Ctrl+C`.

Use a different configuration file with:

```bash
./build/crypto_signal_engine --live --config config/default.toml
```

## Configuration

The default configuration is in [config/default.toml](config/default.toml).

Important strategy settings:

```toml
sampling_interval_ms = 1000
beta = 1.0
rolling_window = 1000
entry_threshold = 2.0
exit_threshold = 0.5
metrics_port = 9100
```

The default rolling window requires approximately 1,000 seconds of one-second samples before the first trading signal can be emitted. This warm-up is intentional.

## Signal Logs

Signals are logged in a readable form:

```text
[strategy] seq=12 pair=BTCUSDT/ETHUSDT action=ENTER_LONG_SPREAD z=-2.31 btc=82700.12 eth=2488.42 valid=true config=v1
```

`ENTER_LONG_SPREAD` means the observed spread z-score is below the configured negative entry threshold. `ENTER_SHORT_SPREAD` means it is above the positive threshold. `EXIT_SPREAD` means the active virtual spread has returned inside the exit threshold.

These are experimental research signals, not exchange orders.

## Metrics

The live process exposes Prometheus text format metrics at:

```text
http://127.0.0.1:9100/metrics
```

Quick check:

```bash
curl http://127.0.0.1:9100/metrics
```

Important metrics include:

- `crypto_feed_messages_total`
- `crypto_parse_failures_total`
- `crypto_sequence_gaps_total`
- `crypto_recoveries_total`
- `crypto_queue_depth`
- `crypto_queue_high_water_mark`
- `crypto_queue_overflows`
- `crypto_order_book_updates_total`
- `crypto_queue_wait_seconds_bucket`
- `crypto_book_update_seconds_bucket`
- `crypto_strategy_seconds_bucket`
- `crypto_signals_total`

The latency histograms are intended to show where processing time accumulates: waiting in the queue, updating books, or evaluating the strategy.

## Prometheus

A scrape configuration is provided at [deploy/prometheus.yml](deploy/prometheus.yml).

For a locally installed Prometheus server, copy or reference that file and start Prometheus with:

```bash
prometheus --config.file=deploy/prometheus.yml
```

The target is `127.0.0.1:9100`. Start the crypto engine before checking the Prometheus targets page:

```bash
./build/crypto_signal_engine --live
```

For Prometheus running inside Docker while the crypto engine runs on the host, change the target to `host.docker.internal:9100`. On Linux, add the host gateway when starting the container if needed:

```bash
docker run --add-host=host.docker.internal:host-gateway \
  -p 9090:9090 \
  -v "$PWD/deploy/prometheus.yml:/etc/prometheus/prometheus.yml" \
  prom/prometheus
```

## Grafana

The dashboard definition is [dashboards/crypto-signal-overview.json](dashboards/crypto-signal-overview.json).

1. Start Prometheus and the crypto engine.
2. Open Grafana and create a Prometheus data source.
3. Set the data source URL to `http://localhost:9090` when Prometheus runs locally.
4. Test and save the data source.
5. Go to **Dashboards**, choose **Import**, and upload `dashboards/crypto-signal-overview.json`.
6. Select the Prometheus data source when Grafana asks for `${DS_PROMETHEUS}`.
7. Open the dashboard and choose a recent time range such as **Last 15 minutes**.

The dashboard includes queue depth, queue overflows, update throughput, p95 book-update latency, p95 queue-wait latency, sequence gaps, recoveries, and signal rates.

## Tests

The test suite covers:

- Configuration and startup smoke behavior
- SPSC queue ordering, capacity, shutdown, and concurrent use
- Binance WebSocket depth parsing
- Binance REST snapshot parsing
- Order-book snapshots, updates, stale events, and sequence gaps
- Mean-reversion warm-up and state transitions
- Metrics rendering and histogram output

Run all tests with:

```bash
ctest --test-dir build --output-on-failure
```

## Known Limitations

- WebSocket shutdown can wait for a blocked network read.
- REST recovery is synchronous in the main processing loop.
- Metrics port conflicts are reported by the process runtime rather than a startup validation step.
- The strategy uses online rolling statistics only and does not establish that BTC/ETH is mean-reverting.
- The implementation intentionally has no TCP signal clients, order placement, or historical backtesting.

## Project Layout

```text
include/crypto/   Public C++ interfaces
src/              Feed, REST, order book, strategy, metrics, and executable code
tests/            Deterministic unit and smoke tests
config/           Runtime TOML configuration
dashboards/       Grafana dashboard JSON
deploy/           Prometheus scrape configuration
docs/             Concurrency and supporting documentation
```
