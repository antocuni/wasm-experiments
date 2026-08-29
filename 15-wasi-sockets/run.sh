#!/bin/bash
# Start the echo server, wait for it to bind, run the client against it, then
# wait for the server to finish. Both run as separate wasmtime processes over
# the host loopback, with WASI preview2 sockets enabled.
#
# Usage: ./run.sh [HOST] [PORT] [MESSAGE]
set -euo pipefail
cd "$(dirname "$0")"
source ./config.sh

HOST=${1:-127.0.0.1}
PORT=${2:-8080}
MSG=${3:-hello from a WASI socket}

if [ ! -f server.wasm ] || [ ! -f client.wasm ]; then
    echo "server.wasm/client.wasm not found; run ./make-zig.sh first" >&2
    exit 1
fi

echo ">>> starting server on $HOST:$PORT"
"$WASMTIME" run -S inherit-network -S tcp server.wasm "$HOST" "$PORT" &
SRV=$!
sleep 1

echo ">>> running client"
"$WASMTIME" run -S inherit-network -S tcp client.wasm "$HOST" "$PORT" "$MSG"

wait "$SRV"
