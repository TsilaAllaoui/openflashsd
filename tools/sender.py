import argparse
import threading
import time

import serial


def read_burst(ser: serial.Serial, idle_time=0.005):
    first = ser.read(1)

    if not first:
        return b""

    data = bytearray(first)

    deadline = time.monotonic() + idle_time

    while True:
        waiting = ser.in_waiting

        if waiting > 0:
            data.extend(ser.read(waiting))
            deadline = time.monotonic() + idle_time
            continue

        if time.monotonic() >= deadline:
            break

        time.sleep(0.001)

    return bytes(data)


def receiver(ser: serial.Serial):
    while ser.is_open:
        try:
            data = read_burst(ser)

            if not data:
                continue

            print()
            print(f"RX HEX : {data.hex(' ').upper()}")

            try:
                text = data.decode("utf-8")
                print(f"RX TEXT: {text!r}")
            except UnicodeDecodeError:
                pass

            print("> ", end="", flush=True)

        except serial.SerialException:
            break


def send_hex(ser: serial.Serial, value: str):
    value = value.replace(" ", "")

    try:
        data = bytes.fromhex(value)
    except ValueError:
        print("Invalid hexadecimal data")
        return

    ser.write(data)
    ser.flush()

    print(f"TX HEX : {data.hex(' ').upper()}")


def send_text(ser: serial.Serial, value: str):
    data = value.encode("utf-8")

    ser.write(data)
    ser.flush()

    print(f"TX TEXT: {value!r}")
    print(f"TX HEX : {data.hex(' ').upper()}")


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--port",
        required=True
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=115200
    )

    args = parser.parse_args()

    ser = serial.Serial(
        port=args.port,
        baudrate=args.baud,
        timeout=0.1
    )

    time.sleep(0.5)

    print(f"Connected to {args.port} @ {args.baud}")
    print()
    print("Commands:")
    print("  text                  Send text")
    print("  /hex 4F 46 01 00      Send hexadecimal bytes")
    print("  /clear                Clear RX buffer")
    print("  /quit                 Exit")
    print()

    thread = threading.Thread(
        target=receiver,
        args=(ser,),
        daemon=True
    )

    thread.start()

    try:
        while True:
            value = input("> ")

            if value == "/quit":
                break

            if value == "/clear":
                ser.reset_input_buffer()
                print("RX buffer cleared")
                continue

            if value.startswith("/hex "):
                send_hex(
                    ser,
                    value[5:]
                )

                continue

            send_text(
                ser,
                value
            )

    except KeyboardInterrupt:
        pass
    finally:
        ser.close()

    print("Disconnected")


if __name__ == "__main__":
    main()