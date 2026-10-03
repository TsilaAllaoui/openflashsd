#!/usr/bin/env bash

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SKETCH="$ROOT/esp32/openflash_esp32"
BUILD="$ROOT/esp32/intellisense-build"
PROTOCOL="$ROOT/protocol"

ARDUINO_CLI="${ARDUINO_CLI:-arduino-cli}"

mkdir -p "$BUILD"

echo "Generating ESP32 IntelliSense compilation database..."
echo

"$ARDUINO_CLI" compile \
    --fqbn esp32:esp32:esp32c3 \
    --board-options CDCOnBoot=cdc \
    --library "$PROTOCOL" \
    --build-path "$BUILD" \
    --only-compilation-database \
    "$SKETCH"

echo
echo "Generated:"
echo "  $BUILD/compile_commands.json"
echo
echo "This build directory is for IntelliSense only."
echo "It is NOT suitable for Windows upload."