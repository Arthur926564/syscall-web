# HTTP/1.1 server from scratch in C

A multi-threaded static-file HTTP/1.1 server written in plain C on Linux, built to learn how the OS, networking and the filesystem actually behave under load, and measured against nginx on the same machine.

## Results

| Build | Throughput (100 KiB files) | p99 latency | Notes |
|---|---|---|---|
| nginx (reference) | ~288k req/s | <!-- TODO: fill in --> | same machine, same session |
| **epoll + sendfile** (`main`) | **~288k req/s** (matches nginx) | 40 ms | per-worker event loops |
| **io_uring** (`feature-io-uring`) | ~40% lower | **7.6 ms** (5× lower) | multishot accept, mmap + SEND |

Benchmark: `wrk -t8 -c400` <!-- TODO: duration, file set -->, runs taken in the same session.
Machine: <!-- TODO: CPU model, core count, RAM, kernel version -->.

The full methodology, profiling steps and the latency/throughput trade-off are in the report: [docs.pdf](https://github.com/user-attachments/files/26648074/docs.pdf).

## How it works

- **Per-worker event loops.** Each worker thread owns its own `epoll` instance and its own listening socket via `SO_REUSEPORT`, so the kernel load-balances connections and there is no cross-thread dispatch or shared queue.
- **Zero-copy file serving.** Static files are sent with `sendfile`, avoiding copies through user space.
- **Keep-alive** connections, with **memory-capped per-client state** so slow clients cannot exhaust the server.
- **Optimized with profiling.** Every change was driven by `perf` profiles and re-benchmarked with `wrk`.

### Layers

| Layer | Role |
|---|---|
| OS | non-blocking reads/writes of raw bytes |
| TCP | accepting connections (`SO_REUSEPORT`, one listener per worker) |
| Buffer | safe handling of partial reads and writes |
| HTTP parser | raw bytes → `http_request_t` (request line, headers, body) |
| Router | static file vs 404 |

## Build and run

Requires **Linux** (uses `epoll`, `sendfile` and `SO_REUSEPORT`; the `io_uring` branch needs a recent kernel), GCC or Clang, and `libmagic` (for MIME types).

```bash
# bash: enable ** globbing first
shopt -s globstar
gcc -O2 -g -fno-omit-frame-pointer -pthread src/**/*.c -Iinclude -lmagic -o my_server
./my_server
```

By default the server listens on `http://localhost:8080`.

## Limitations

- No HTTPS
- No chunked transfer encoding
- No IPv6
- Security hardening still needed (do not expose it to the internet)

## Possible next steps

- Directory listing
- Basic API endpoints
- IPv6 support
- Security hardening
