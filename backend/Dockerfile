# ── build stage ──────────────────────────────────────────────────────────────
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake git ca-certificates \
        libeigen3-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

# ── runtime stage ─────────────────────────────────────────────────────────────
FROM debian:bookworm-slim

# libdatachannel links against OpenSSL at runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
        libssl3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/server /usr/local/bin/server

EXPOSE 8080
# UDP port range used by libdatachannel ICE candidates
EXPOSE 10000-10010/udp

CMD ["server"]
