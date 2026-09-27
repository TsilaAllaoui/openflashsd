import argparse
import struct
import threading
import time

import serial


MAGIC = b"OF"
PROTOCOL_VERSION = 1

COMMAND_PING = 0x01
COMMAND_GET_CART_INFO = 0x02
COMMAND_LIST_FILES = 0x03

STATUS_OK = 0x00

STATUS_NAMES = {
    0x00: "OK",
    0x01: "ERROR",
    0x02: "INVALID_COMMAND",
    0x03: "INVALID_PAYLOAD",
    0x04: "NOT_FOUND",
    0x05: "NOT_READY",
}

FILE_TYPES = {
    0: "FILE",
    1: "DIR ",
    2: "GBA ",
    3: "SAVE",
}

SAVE_TYPES = {
    0: "NONE",
    1: "EEPROM_512B",
    2: "EEPROM_8K",
    3: "SRAM_32K",
    4: "FLASH_64K",
    5: "FLASH_128K",
    6: "FRAM_32K",
    7: "FRAM_64K",
    8: "FRAM_128K",
    9: "UNKNOWN",
}


def calculate_crc16(data: bytes) -> int:
    crc = 0xFFFF

    for byte in data:
        crc ^= byte << 8

        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF

    return crc


def build_packet(
    sequence: int,
    command: int,
    payload: bytes = b""
) -> bytes:
    packet = bytearray()

    packet += MAGIC
    packet.append(PROTOCOL_VERSION)
    packet.append(sequence & 0xFF)
    packet.append(command & 0xFF)

    packet += struct.pack(
        "<H",
        len(payload)
    )

    packet += payload

    crc = calculate_crc16(
        packet
    )

    packet += struct.pack(
        "<H",
        crc
    )

    return bytes(packet)


def encode_string(value: str) -> bytes:
    encoded = value.encode("utf-8")

    return (
        struct.pack(
            "<H",
            len(encoded)
        ) +
        encoded
    )


def read_u8(
    data: bytes,
    offset: int
):
    if offset + 1 > len(data):
        raise ValueError(
            "Unexpected end while reading uint8"
        )

    return (
        data[offset],
        offset + 1
    )


def read_u16(
    data: bytes,
    offset: int
):
    if offset + 2 > len(data):
        raise ValueError(
            "Unexpected end while reading uint16"
        )

    value = struct.unpack_from(
        "<H",
        data,
        offset
    )[0]

    return (
        value,
        offset + 2
    )


def read_u32(
    data: bytes,
    offset: int
):
    if offset + 4 > len(data):
        raise ValueError(
            "Unexpected end while reading uint32"
        )

    value = struct.unpack_from(
        "<I",
        data,
        offset
    )[0]

    return (
        value,
        offset + 4
    )


def read_string(
    data: bytes,
    offset: int
):
    length, offset = read_u16(
        data,
        offset
    )

    if offset + length > len(data):
        raise ValueError(
            f"String length {length} "
            f"exceeds payload size"
        )

    raw = data[
        offset:
        offset + length
    ]

    try:
        value = raw.decode(
            "utf-8"
        )
    except UnicodeDecodeError:
        value = raw.decode(
            "latin-1"
        )

    return (
        value,
        offset + length
    )


def status_name(
    value: int
) -> str:
    return STATUS_NAMES.get(
        value,
        f"UNKNOWN_STATUS ({value})"
    )


def human_size(
    size: int
) -> str:
    if size < 1024:
        return f"{size} B"

    if size < 1024 * 1024:
        return (
            f"{size / 1024:.1f} KB"
        )

    if size < 1024 * 1024 * 1024:
        return (
            f"{size / (1024 * 1024):.1f} MB"
        )

    return (
        f"{size / (1024 * 1024 * 1024):.2f} GB"
    )


def parse_packet(
    data: bytes
):
    if len(data) < 9:
        raise ValueError(
            "Packet too small"
        )

    if data[0:2] != MAGIC:
        raise ValueError(
            "Invalid magic"
        )

    version = data[2]
    sequence = data[3]
    command = data[4]

    payload_size = struct.unpack_from(
        "<H",
        data,
        5
    )[0]

    expected_size = (
        7 +
        payload_size +
        2
    )

    if len(data) != expected_size:
        raise ValueError(
            f"Invalid packet size: "
            f"expected {expected_size}, "
            f"got {len(data)}"
        )

    received_crc = struct.unpack_from(
        "<H",
        data,
        len(data) - 2
    )[0]

    calculated_crc = calculate_crc16(
        data[:-2]
    )

    if received_crc != calculated_crc:
        raise ValueError(
            f"CRC mismatch: "
            f"received 0x{received_crc:04X}, "
            f"calculated 0x{calculated_crc:04X}"
        )

    payload = data[
        7:
        7 + payload_size
    ]

    return {
        "version": version,
        "sequence": sequence,
        "command": command,
        "payload": payload,
        "crc": received_crc,
    }


def decode_ping(
    payload: bytes
):
    try:
        status, offset = read_u8(
            payload,
            0
        )

    except ValueError as error:
        print(
            f"Invalid PING payload: {error}"
        )
        return

    print()
    print("PING RESPONSE")
    print("-------------")
    print(
        f"Status : {status_name(status)}"
    )

    if offset != len(payload):
        print(
            f"Extra  : "
            f"{payload[offset:].hex(' ').upper()}"
        )


def decode_cart_infos(
    payload: bytes
):
    offset = 0

    try:
        status, offset = read_u8(
            payload,
            offset
        )

        if status != STATUS_OK:
            print()
            print("CART INFORMATION")
            print("----------------")
            print(
                f"Status : {status_name(status)}"
            )
            return

        file_path, offset = read_string(
            payload,
            offset
        )

        name, offset = read_string(
            payload,
            offset
        )

        game_code, offset = read_string(
            payload,
            offset
        )

        maker_code, offset = read_string(
            payload,
            offset
        )

        header_valid, offset = read_u8(
            payload,
            offset
        )

        save_type_value, offset = read_u8(
            payload,
            offset
        )

    except ValueError as error:
        print()
        print(
            f"Invalid CART_INFO payload: "
            f"{error}"
        )
        return

    print()
    print("CART INFORMATION")
    print("----------------")
    print("Status       : OK")
    print(
        f"File path    : {file_path}"
    )
    print(
        f"Name         : {name}"
    )
    print(
        f"Game code    : {game_code}"
    )
    print(
        f"Maker code   : {maker_code}"
    )
    print(
        f"Header valid : "
        f"{'Yes' if header_valid else 'No'}"
    )
    print(
        f"Save type    : "
        f"{SAVE_TYPES.get(save_type_value, f'UNKNOWN ({save_type_value})')}"
    )

    if offset != len(payload):
        print(
            f"Extra bytes  : "
            f"{payload[offset:].hex(' ').upper()}"
        )


def decode_file_listing(
    payload: bytes
):
    offset = 0

    try:
        status, offset = read_u8(
            payload,
            offset
        )

        print()
        print("FILE LISTING")
        print("------------")
        print(
            f"Status  : {status_name(status)}"
        )

        if status != STATUS_OK:
            return

        entry_count, offset = read_u16(
            payload,
            offset
        )

        entries = []

        for _ in range(entry_count):
            file_type, offset = read_u8(
                payload,
                offset
            )

            file_size, offset = read_u32(
                payload,
                offset
            )

            path, offset = read_string(
                payload,
                offset
            )

            entries.append({
                "type": file_type,
                "size": file_size,
                "path": path,
            })

    except ValueError as error:
        print(
            f"Invalid LIST_FILES payload: "
            f"{error}"
        )
        return

    print(
        f"Entries : {entry_count}"
    )
    print()

    if not entries:
        print(
            "<empty directory>"
        )
        return

    for index, entry in enumerate(
        entries,
        start=1
    ):
        type_value = entry["type"]

        type_text = FILE_TYPES.get(
            type_value,
            f"?{type_value:02X}"
        )

        if type_value == 1:
            size_text = ""
        else:
            size_text = human_size(
                entry["size"]
            )

        line = (
            f"{index:3}. "
            f"[{type_text}] "
            f"{entry['path']}"
        )

        if size_text:
            line += (
                f"  {size_text}"
            )

        print(line)

    if offset != len(payload):
        print()
        print(
            f"Extra bytes: "
            f"{payload[offset:].hex(' ').upper()}"
        )


def display_packet(
    data: bytes
):
    print()
    print(
        f"RX HEX : "
        f"{data.hex(' ').upper()}"
    )

    try:
        packet = parse_packet(
            data
        )

    except ValueError as error:
        print(
            f"RX ERROR: {error}"
        )
        return

    print(
        f"Version  : "
        f"{packet['version']}"
    )

    print(
        f"Sequence : "
        f"{packet['sequence']}"
    )

    print(
        f"Command  : "
        f"0x{packet['command']:02X}"
    )

    print(
        f"CRC      : "
        f"0x{packet['crc']:04X} OK"
    )

    if packet["command"] == COMMAND_PING:
        decode_ping(
            packet["payload"]
        )

    elif packet["command"] == COMMAND_GET_CART_INFO:
        decode_cart_infos(
            packet["payload"]
        )

    elif packet["command"] == COMMAND_LIST_FILES:
        decode_file_listing(
            packet["payload"]
        )

    else:
        print(
            f"Payload  : "
            f"{packet['payload'].hex(' ').upper()}"
        )


class PacketStreamParser:
    def __init__(self):
        self.buffer = bytearray()

    def reset(self):
        self.buffer.clear()

    def push(
        self,
        data: bytes
    ):
        self.buffer.extend(
            data
        )

        packets = []

        while True:
            while (
                self.buffer and
                self.buffer[0] != MAGIC[0]
            ):
                del self.buffer[0]

            if len(self.buffer) < 2:
                break

            if self.buffer[1] != MAGIC[1]:
                del self.buffer[0]
                continue

            if len(self.buffer) < 7:
                break

            payload_size = (
                self.buffer[5] |
                (self.buffer[6] << 8)
            )

            expected_size = (
                7 +
                payload_size +
                2
            )

            if len(self.buffer) < expected_size:
                break

            candidate = bytes(
                self.buffer[
                    :expected_size
                ]
            )

            try:
                parse_packet(
                    candidate
                )

                packets.append(
                    candidate
                )

                del self.buffer[
                    :expected_size
                ]

            except ValueError:
                del self.buffer[0]

        return packets


def receiver(
    ser: serial.Serial,
    stop_event: threading.Event
):
    stream_parser = PacketStreamParser()

    while (
        ser.is_open and
        not stop_event.is_set()
    ):
        try:
            waiting = ser.in_waiting

            if waiting <= 0:
                time.sleep(
                    0.001
                )
                continue

            data = ser.read(
                waiting
            )

            if not data:
                continue

            packets = stream_parser.push(
                data
            )

            for packet in packets:
                display_packet(
                    packet
                )

                print(
                    "> ",
                    end="",
                    flush=True
                )

        except serial.SerialException:
            break


def send_packet(
    ser: serial.Serial,
    sequence: int,
    command: int,
    payload: bytes = b""
):
    data = build_packet(
        sequence,
        command,
        payload
    )

    ser.write(
        data
    )

    ser.flush()

    print(
        f"TX HEX : "
        f"{data.hex(' ').upper()}"
    )


def send_hex(
    ser: serial.Serial,
    value: str
):
    value = value.replace(
        " ",
        ""
    )

    try:
        data = bytes.fromhex(
            value
        )

        crc16 = calculate_crc16(
            data
        )

        data += crc16.to_bytes(
            2,
            byteorder="little"
        )

    except ValueError:
        print(
            "Invalid hexadecimal data"
        )
        return

    ser.write(
        data
    )

    ser.flush()

    print(
        f"TX HEX : "
        f"{data.hex(' ').upper()}"
    )


def send_text(
    ser: serial.Serial,
    value: str
):
    data = value.encode(
        "utf-8"
    )

    ser.write(
        data
    )

    ser.flush()

    print(
        f"TX TEXT: "
        f"{value!r}"
    )

    print(
        f"TX HEX : "
        f"{data.hex(' ').upper()}"
    )


def main():
    argument_parser = argparse.ArgumentParser()

    argument_parser.add_argument(
        "--port",
        required=True
    )

    argument_parser.add_argument(
        "--baud",
        type=int,
        default=115200
    )

    args = argument_parser.parse_args()

    ser = serial.Serial(
        port=args.port,
        baudrate=args.baud,
        timeout=0.05
    )

    time.sleep(
        0.5
    )

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print(
        f"Connected to "
        f"{args.port} @ {args.baud}"
    )

    print()
    print("Commands:")
    print("  /ping                 Send PING")
    print("  /cart                 Send GET_CART_INFO")
    print("  /ls                   List root directory")
    print("  /ls /GAMES            List /GAMES")
    print("  /ls /SAVES            List /SAVES")
    print("  /hex 4F 46 ...        Send raw body + append CRC")
    print("  /clear                Clear RX buffer")
    print("  /quit                 Exit")
    print("  anything else         Send raw text")
    print()

    sequence = 0

    stop_event = threading.Event()

    thread = threading.Thread(
        target=receiver,
        args=(
            ser,
            stop_event
        ),
        daemon=True
    )

    thread.start()

    try:
        while True:
            value = input(
                "> "
            )

            value = value.strip()

            if value == "/quit":
                break

            if value == "/clear":
                ser.reset_input_buffer()

                print(
                    "RX buffer cleared"
                )

                continue

            if value == "/ping":
                send_packet(
                    ser,
                    sequence,
                    COMMAND_PING
                )

                sequence = (
                    sequence + 1
                ) & 0xFF

                continue

            if value == "/cart":
                send_packet(
                    ser,
                    sequence,
                    COMMAND_GET_CART_INFO
                )

                sequence = (
                    sequence + 1
                ) & 0xFF

                continue

            if value == "/ls":
                path = "/"

                payload = encode_string(
                    path
                )

                send_packet(
                    ser,
                    sequence,
                    COMMAND_LIST_FILES,
                    payload
                )

                sequence = (
                    sequence + 1
                ) & 0xFF

                continue

            if value.startswith(
                "/ls "
            ):
                path = value[
                    4:
                ].strip()

                if not path:
                    path = "/"

                if not path.startswith("/"):
                    path = (
                        "/" +
                        path
                    )

                payload = encode_string(
                    path
                )

                send_packet(
                    ser,
                    sequence,
                    COMMAND_LIST_FILES,
                    payload
                )

                sequence = (
                    sequence + 1
                ) & 0xFF

                continue

            if value.startswith(
                "/hex "
            ):
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
        stop_event.set()

        if ser.is_open:
            ser.close()

        thread.join(
            timeout=0.5
        )

    print(
        "Disconnected"
    )


if __name__ == "__main__":
    main()