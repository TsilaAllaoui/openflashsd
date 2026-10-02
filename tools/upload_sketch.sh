#!/usr/bin/env bash

echo "[1/2] Terminating any process using the serial port..."

# Kill common background CLI tools that lock ports from WSL
# Note the explicit .exe extension so WSL calls the Windows host binaries
taskkill.exe /F /IM python.exe /T 2>/dev/null
taskkill.exe /F /IM esptool.exe /T 2>/dev/null
taskkill.exe /F /IM putty.exe /T 2>/dev/null

# Wait 1 second to ensure Windows releases the OS handle
sleep 1

# Compile and upload the firmware to the ESP32
ROOT="$(pwd)"

SKETCH="$(wslpath -w "$ROOT/esp32/openflash_esp32")"
PROTOCOL="$(wslpath -w "$ROOT/protocol")"

pushd /mnt/c >/dev/null

"$ARDUINO_CLI" compile \
    --fqbn esp32:esp32:esp32c3 \
    --board-options CDCOnBoot=cdc \
    --library "$PROTOCOL" \
    --jobs 11 \
    --upload \
    --port COM20 \
    "$SKETCH"

# Monitor serial

"$ARDUINO_CLI" monitor \
    -p COM20 \
    --config baudrate=115200

popd >/dev/null