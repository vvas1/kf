# ── build stage ──────────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake libeigen3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# ── runtime stage ─────────────────────────────────────────────────────────────
FROM debian:bookworm-slim

COPY --from=builder /src/build/server /usr/local/bin/server

EXPOSE 8080
CMD ["server"]
