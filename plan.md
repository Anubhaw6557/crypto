# Live Crypto Signal Engine

## Objective

Build a modular, low-latency C++20 service that consumes Binance Spot market data, maintains local order books, generates experimental online BTC/ETH mean-reversion signals, and publishes those signals to external observers over TCP.

The system does not place orders, manage exchange execution, backtest historical data, or claim profitability.

## Definition of Done

- [ ] The service builds reproducibly with CMake and C++20 on Linux.
- [ ] Configuration, logs, metrics, health state, and graceful shutdown are operational.
- [ ] BTCUSDT, ETHUSDT, SOLUSDT, BNBUSDT, and XRPUSDT feeds can connect, reconnect, and recover.
- [ ] Each symbol has a sequence-validated local order book with freshness and validity state.
- [ ] BTC/ETH sampling, warm-up, rolling statistics, stateful signals, and suppression rules are tested.
- [ ] Multiple TCP observers receive versioned newline-delimited JSON without blocking feed processing.
- [ ] Prometheus metrics, Grafana dashboards, benchmarks, stress tests, and profiling instructions are included.
- [ ] No code path submits exchange orders or treats TCP write completion as observer receipt.

## Technology and Constraints

- [ ] C++20, CMake, Boost.Asio, Boost.Beast, OpenSSL, simdjson, toml++, spdlog.
- [ ] prometheus-cpp for metrics, Catch2 for unit tests, Google Benchmark for benchmarks, Linux `perf` for profiling.
- [ ] Use monotonic clocks for elapsed-time measurements and wall-clock timestamps only for external event time.
- [ ] Keep market-data processing independent from observer I/O and bound every queue.
- [ ] Make exchange endpoints, credentials if ever added, symbols, intervals, thresholds, and limits configurable.

## Phase 0: Repository and Foundation

- [x] Add a top-level `CMakeLists.txt` with Debug, Release, and sanitizer options.
- [x] Create `src/`, `include/`, `tests/`, `benchmarks/`, `config/`, `deploy/`, `dashboards/`, and `docs/` directories.
- [ ] Add dependency discovery and pin/document supported library versions.
- [ ] Define module boundaries: `feed`, `orderbook`, `strategy`, `transport`, `observability`, and `common`.
- [ ] Add a typed TOML configuration with validation and safe defaults.
- [ ] Add structured logging with severity, component, symbol, and correlation fields.
- [ ] Add signal handling, bounded shutdown, and clear startup failure messages.
- [x] Add a minimal executable that starts, reports health, and shuts down cleanly.
- [x] Add formatting, compiler warnings, and a test target to the default build.

**Acceptance:** A clean checkout configures, builds, runs its smoke test, and exits cleanly without network access.

## Phase 1: Shared Data Contracts and Queues

- [ ] Define normalized market-data types for depth updates, snapshots, timestamps, and feed health.
- [ ] Define immutable signal and health-status types with schema/version fields.
- [ ] Implement a bounded SPSC ring buffer with explicit overflow behavior and metrics hooks.
- [ ] Add tests for FIFO ordering, capacity boundaries, empty/full behavior, and shutdown behavior.
- [ ] Document thread ownership and permitted transitions for every cross-thread queue.

**Acceptance:** Queue and data-contract tests pass under ThreadSanitizer where supported.

## Phase 2: Binance Market-Data Ingestion

- [ ] Implement WebSocket connection, TLS setup, subscriptions, heartbeat handling, and orderly close.
- [ ] Subscribe to depth streams for BTCUSDT, ETHUSDT, SOLUSDT, BNBUSDT, and XRPUSDT.
- [ ] Parse messages with simdjson and normalize exchange-specific fields.
- [ ] Validate required fields, symbol, event time, update IDs, and quantities/prices before enqueueing.
- [ ] Implement REST snapshot retrieval with timeout, retry, backoff, and response validation.
- [ ] Implement reconnect handling with bounded exponential backoff and jitter.
- [ ] Track per-symbol receive time, parse failures, reconnects, sequence gaps, and recovery time.
- [ ] Add configurable stale-data thresholds and a feed-health state machine.
- [ ] Add integration tests using recorded fixtures or a local fake WebSocket/REST server; do not depend on live credentials.

**Acceptance:** Fixture-driven tests prove valid messages are normalized, malformed messages are rejected, and reconnects restore feed health.

## Phase 3: Local Order Books

- [ ] Implement per-symbol bid and ask price-level storage with deterministic update and delete semantics.
- [ ] Apply snapshots and buffer or discard updates according to Binance synchronization rules.
- [ ] Validate update-ID continuity and reject stale, duplicated, or gapped updates.
- [ ] Trigger recovery on sequence gaps, invalid snapshots, or inconsistent book state.
- [ ] Expose best bid, best ask, mid-price, spread, update timestamp, and freshness state.
- [ ] Ensure invalid or stale books cannot provide strategy inputs.
- [ ] Add unit tests for insertion, replacement, deletion, empty sides, crossed books, snapshots, and gaps.
- [ ] Benchmark update application and best-price lookup at realistic book sizes.

**Acceptance:** Order-book tests cover normal synchronization and recovery, and no invalid book produces a valid market-data input.

## Phase 4: Online BTC/ETH Strategy

- [ ] Sample synchronized BTC and ETH mid-prices at a configurable one-second interval.
- [ ] Calculate `S_t = ln(P_BTC,t) - beta * ln(P_ETH,t)` with configurable `beta`, initially `1.0`.
- [ ] Maintain a rolling buffer of `1,000` spread observations with a clearly defined full-window policy.
- [ ] Calculate rolling mean, standard deviation, and `z_t = (S_t - mu_t) / sigma_t` with numerically safe handling of zero variance.
- [ ] Implement warm-up so no trading signal is emitted before the configured window is ready.
- [ ] Implement a state machine: neutral, long spread, and short spread; prevent repeated entry signals.
- [ ] Emit `ENTER_LONG_SPREAD` when `z < -2` and `ENTER_SHORT_SPREAD` when `z > +2`.
- [ ] Emit `EXIT_SPREAD` for an active position when `abs(z) < 0.5`.
- [ ] Suppress entries and publish health status when either book is stale, invalid, unsynchronized, or unavailable.
- [ ] Include reference prices, z-score, spread, timestamps, configuration version, and validity in each signal.
- [ ] Add tests for warm-up, thresholds, transitions, repeated samples, zero variance, stale inputs, and recovery.

**Acceptance:** Deterministic tests demonstrate every state transition and prove invalid inputs cannot create entry signals.

## Phase 5: TCP Signal Service

- [ ] Define versioned newline-delimited JSON for signals and health events.
- [ ] Implement an asynchronous Boost.Asio TCP listener with multiple simultaneous observers.
- [ ] Give each client an independent bounded outbound queue and write strand/executor.
- [ ] Ensure slow, disconnected, or overflowing clients cannot block market-data or strategy threads.
- [ ] Define and test client overflow behavior, disconnect cleanup, partial writes, and reconnects.
- [ ] Add connection, backlog, overflow, write-duration, and write-error metrics.
- [ ] Document that successful TCP write completion only means the kernel accepted the bytes.
- [ ] Add protocol and failure tests with local fake clients.

**Acceptance:** A slow client is isolated while healthy clients continue receiving ordered messages and the producer remains responsive.

## Phase 6: Observability and Operations

- [ ] Instrument receive, parse, queue, book update, strategy evaluation, signal creation, and TCP write stages.
- [ ] Record feed-to-signal and signal-to-write latency histograms with p50, p95, p99, and maximum views.
- [ ] Record throughput, queue depth, wait time, high-water marks, and overflows.
- [ ] Record per-symbol feed age, sequence gaps, reconnects, recovery duration, and current health.
- [ ] Record order-book duration, strategy duration, client backlog, write duration, errors, CPU, and memory.
- [ ] Expose Prometheus metrics on a separate local HTTP endpoint with bounded label cardinality.
- [ ] Add Grafana dashboards for feed health, latency, throughput, queues, books, strategy, and observers.
- [ ] Add runbooks for stale feeds, sequence recovery, observer overload, and graceful shutdown.

**Acceptance:** A local run produces useful metrics and dashboards without exposing secrets or unbounded per-event labels.

## Phase 7: Performance, Reliability, and Release

- [ ] Add Google Benchmarks for parsing, order-book updates, rolling statistics, queues, and signal publication.
- [ ] Add load tests for all five symbols, bursty updates, reconnect storms, and many observers.
- [ ] Measure end-to-end latency and report p50, p95, p99, maximum, throughput, and drop/overflow counts.
- [ ] Profile representative workloads with Linux `perf` and document reproducible commands.
- [ ] Run AddressSanitizer, UndefinedBehaviorSanitizer, and ThreadSanitizer test jobs where compatible.
- [ ] Test controlled shutdown, network loss, malformed data, sequence gaps, REST failures, and TCP failures.
- [ ] Add CI for build, unit tests, static checks, sanitizers, and benchmark smoke tests.
- [ ] Review resource limits, logging volume, configuration secrets, and operational defaults.
- [ ] Write a final architecture document and explicitly record known strategy limitations.

**Acceptance:** Release checks are reproducible, failure modes are tested, and performance results include workload and environment details.

## Initial Configuration Checklist

- [ ] Symbols: BTCUSDT, ETHUSDT, SOLUSDT, BNBUSDT, XRPUSDT.
- [ ] Strategy pair: BTCUSDT/ETHUSDT.
- [ ] Sampling interval: 1 second.
- [ ] Beta: 1.0.
- [ ] Rolling window: 1,000 observations.
- [ ] Entry thresholds: -2 and +2.
- [ ] Exit threshold: absolute z-score below 0.5.
- [ ] Feed and book staleness thresholds: selected, documented, and tested.
- [ ] Queue capacities, client limits, reconnect backoff, and timeouts: selected, documented, and tested.

## Explicit Exclusions

- Exchange order placement or execution.
- Historical backtesting or historical strategy research.
- Private exchange trading APIs.
- Profitability claims without independent validation.