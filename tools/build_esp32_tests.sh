#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JOBS="${JOBS:-$(nproc)}"

cmake -S "$ROOT/esp32" -B "$ROOT/esp32/build"
cmake --build "$ROOT/esp32/build" -j "$JOBS"
ctest --test-dir "$ROOT/esp32/build" --output-on-failure
