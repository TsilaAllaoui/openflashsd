# Building and running esp32 side

- To build use: `arduino-cli compile --fqbn "esp32:esp32:esp32c3:CDCOnBoot=cdc" --build-path build/esp32 openflash_esp32`
    - or just use something like:
    ```
    @echo off
    SET PORT=COM20
    SET BAUD=115200
    SET FIRMWARE_BIN=build/firmware.bin

    echo [1/2] Terminating any process using the serial port...
    :: Kill common background CLI tools that lock ports
    taskkill /F /IM python.exe /T 2>nul
    taskkill /F /IM esptool.exe /T 2>nul
    taskkill /F /IM putty.exe /T 2>nul
    :: Wait 1 second to ensure Windows releases the OS handle
    timeout /t 1 /nobreak >nul

    echo [2/2] Flashing ESP32-C3...
    arduino-cli compile --fqbn "esp32:esp32:esp32c3:CDCOnBoot=cdc" --upload --port COM20 --build-path build/esp32 openflash_esp32

    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Flashing failed. Exiting without opening monitor.
        pause
        exit /b %ERRORLEVEL%
    )
    ```
- Use `tools/sender.py` to send packet bytes to esp32 side and test