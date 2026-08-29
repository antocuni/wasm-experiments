#!/bin/bash
# Remove build artifacts.
set -euo pipefail
cd "$(dirname "$0")"
rm -f server.wasm client.wasm server.o client.o zig-wasm-ld
echo "cleaned"
