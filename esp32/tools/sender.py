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

COMMAND_NAMES = {
    COMMAND_PING: "PING",
    COMMAND_GET_CART_INFO: "GET_CART_INFO",
    COMMAND_LIST_FILES: "LIST_FILES",
}

STATUS_OK = 0x00
STATUS_NOT_FINISHED_YET = 0x06
STATUS_NAMES = {
    0x00: "OK",
    0x01: "ERROR",
    0x02: "INVALID_COMMAND",
    0x03: "INVALID_PAYLOAD",
    0x04: "NOT_FOUND",
    0x05: "NOT_READY",
    0x06: "NOT_FINISHED_YET",
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


def build_packet(sequence: int, command: int, payload: bytes = b"") -> bytes:
    packet = bytearray()
    packet += MAGIC
    packet.append(PROTOCOL_VERSION)
    packet.append(sequence & 0xFF)
    packet.append(command & 0xFF)
    packet += struct.pack("<H", len(payload))
    packet += payload

    crc = calculate_crc16(packet)
    packet += struct.pack("<H", crc)

    return bytes(packet)


def encode_string(value: str) -> bytes:
    encoded = value.encode("utf-8")
    return struct.pack("<H", len(encoded)) + encoded


def read_u8(data: bytes, offset: int):
    if offset + 1 > len(data):
        raise ValueError("Unexpected end while reading uint8")

    return data[offset], offset + 1


def read_u16(data: bytes, offset: int):
    if offset + 2 > len(data):
        raise ValueError("Unexpected end while reading uint16")

    value = struct.unpack_from("<H", data, offset)[0]
    return value, offset + 2


def read_u32(data: bytes, offset: int):
    if offset + 4 > len(data):
        raise ValueError("Unexpected end while reading uint32")

    value = struct.unpack_from("<I", data, offset)[0]
    return value, offset + 4


def read_string(data: bytes, offset: int):
    length, offset = read_u16(data, offset)

    if offset + length > len(data):
        raise ValueError(f"String length {length} exceeds payload size")

    raw = data[offset:offset + length]

    try:
        value = raw.decode("utf-8")
    except UnicodeDecodeError:
        value = raw.decode("latin-1")

    return value, offset + length


def status_name(value: int) -> str:
    return STATUS_NAMES.get(value, f"UNKNOWN_STATUS ({value})")


def human_size(size: int) -> str:
    if size < 1024:
        return f"{size} B"

    if size < 1024 * 1024:
        return f"{size / 1024:.1f} KB"

    if size < 1024 * 1024 * 1024:
        return f"{size / (1024 * 1024):.1f} MB"

    return f"{size / (1024 * 1024 * 1024):.2f} GB"


def format_duration(seconds: float) -> str:
    if seconds < 0.001:
        return f"{seconds * 1_000_000:.0f} us"

    if seconds < 1.0:
        return f"{seconds * 1000:.3f} ms"

    return f"{seconds:.3f} s"


def parse_packet(data: bytes):
    if len(data) < 9:
        raise ValueError("Packet too small")

    if data[0:2] != MAGIC:
        raise ValueError("Invalid magic")

    version = data[2]
    sequence = data[3]
    command = data[4]
    payload_size = struct.unpack_from("<H", data, 5)[0]
    expected_size = 7 + payload_size + 2

    if len(data) != expected_size:
        raise ValueError(f"Invalid packet size: expected {expected_size}, got {len(data)}")

    received_crc = struct.unpack_from("<H", data, len(data) - 2)[0]
    calculated_crc = calculate_crc16(data[:-2])

    if received_crc != calculated_crc:
        raise ValueError(
            f"CRC mismatch: received 0x{received_crc:04X}, calculated 0x{calculated_crc:04X}"
        )

    payload = data[7:7 + payload_size]

    return {
        "version": version,
        "sequence": sequence,
        "command": command,
        "payload": payload,
        "crc": received_crc,
    }


def decode_ping(payload: bytes):
    try:
        status, offset = read_u8(payload, 0)
    except ValueError as error:
        print(f"Invalid PING payload: {error}")
        return

    print()
    print("PING RESPONSE")
    print("-------------")
    print(f"Status : {status_name(status)}")

    if offset != len(payload):
        print(f"Extra  : {payload[offset:].hex(' ').upper()}")


def decode_cart_infos(payload: bytes):
    offset = 0

    try:
        status, offset = read_u8(payload, offset)

        if status != STATUS_OK:
            print()
            print("CART INFORMATION")
            print("----------------")
            print(f"Status : {status_name(status)}")
            return

        file_path, offset = read_string(payload, offset)
        name, offset = read_string(payload, offset)
        game_code, offset = read_string(payload, offset)
        maker_code, offset = read_string(payload, offset)
        header_valid, offset = read_u8(payload, offset)
        save_type_value, offset = read_u8(payload, offset)
    except ValueError as error:
        print()
        print(f"Invalid CART_INFO payload: {error}")
        return

    print()
    print("CART INFORMATION")
    print("----------------")
    print("Status       : OK")
    print(f"File path    : {file_path}")
    print(f"Name         : {name}")
    print(f"Game code    : {game_code}")
    print(f"Maker code   : {maker_code}")
    print(f"Header valid : {'Yes' if header_valid else 'No'}")
    print(f"Save type    : {SAVE_TYPES.get(save_type_value, f'UNKNOWN ({save_type_value})')}")

    if offset != len(payload):
        print(f"Extra bytes  : {payload[offset:].hex(' ').upper()}")


def decode_file_listing(payload: bytes):
    if not payload:
        print("Invalid LIST_FILES payload: empty payload")
        return None

    if len(payload) == 1:
        return {
            "status": payload[0],
            "entries": None,
        }

    offset = 0

    try:
        status, offset = read_u8(payload, offset)

        if status not in (STATUS_OK, STATUS_NOT_FINISHED_YET):
            raise ValueError(f"Unexpected LIST_FILES status {status}")

        data_size, offset = read_u16(payload, offset)
        entry_count, offset = read_u16(payload, offset)

        actual_data_size = len(payload) - offset

        if data_size != actual_data_size:
            print(
                f"LIST_FILES size warning: server declared {data_size} data bytes, "
                f"received {actual_data_size}"
            )

        entries = []

        for _ in range(entry_count):
            file_type, offset = read_u8(payload, offset)
            file_size, offset = read_u32(payload, offset)
            path, offset = read_string(payload, offset)

            entries.append({
                "type": file_type,
                "size": file_size,
                "path": path,
            })

        if offset != len(payload):
            raise ValueError(f"Extra bytes: {payload[offset:].hex(' ').upper()}")
    except ValueError as error:
        print(f"Invalid LIST_FILES payload: {error}")
        return None

    return {
        "status": status,
        "entries": entries,
    }


def display_packet(data: bytes, verbose: bool = False):
    try:
        packet = parse_packet(data)
    except ValueError as error:
        print(f"RX ERROR: {error}")
        return None

    if verbose:
        print()
        print(f"RX HEX : {data.hex(' ').upper()}")
        print(f"Version  : {packet['version']}")
        print(f"Sequence : {packet['sequence']}")
        print(f"Command  : 0x{packet['command']:02X}")
        print(f"CRC      : 0x{packet['crc']:04X} OK")

    if packet["command"] == COMMAND_PING:
        decode_ping(packet["payload"])
    elif packet["command"] == COMMAND_GET_CART_INFO:
        decode_cart_infos(packet["payload"])
    elif packet["command"] != COMMAND_LIST_FILES and verbose:
        print(f"Payload  : {packet['payload'].hex(' ').upper()}")

    return packet


class PacketStreamParser:
    def __init__(self):
        self.buffer = bytearray()

    def reset(self):
        self.buffer.clear()

    def push(self, data: bytes):
        self.buffer.extend(data)
        packets = []

        while True:
            while self.buffer and self.buffer[0] != MAGIC[0]:
                del self.buffer[0]

            if len(self.buffer) < 2:
                break

            if self.buffer[1] != MAGIC[1]:
                del self.buffer[0]
                continue

            if len(self.buffer) < 7:
                break

            payload_size = self.buffer[5] | (self.buffer[6] << 8)
            expected_size = 7 + payload_size + 2

            if len(self.buffer) < expected_size:
                break

            candidate = bytes(self.buffer[:expected_size])

            try:
                parse_packet(candidate)
                packets.append(candidate)
                del self.buffer[:expected_size]
            except ValueError:
                del self.buffer[0]

        return packets


class SequenceCounter:
    def __init__(self):
        self._value = 0
        self._lock = threading.Lock()

    def next(self) -> int:
        with self._lock:
            value = self._value
            self._value = (self._value + 1) & 0xFF
            return value


class RequestMetrics:
    def __init__(self):
        self._requests = {}
        self._lock = threading.Lock()

    def start(self, sequence: int, command: int, started_at: float = None):
        if started_at is None:
            started_at = time.perf_counter()

        with self._lock:
            self._requests[(sequence, command)] = started_at

        return started_at

    def elapsed(self, sequence: int, command: int, finished_at: float = None):
        if finished_at is None:
            finished_at = time.perf_counter()

        with self._lock:
            started_at = self._requests.get((sequence, command))

        if started_at is None:
            return None

        return finished_at - started_at

    def finish(self, sequence: int, command: int, finished_at: float = None):
        if finished_at is None:
            finished_at = time.perf_counter()

        with self._lock:
            started_at = self._requests.pop((sequence, command), None)

        if started_at is None:
            return None

        return finished_at - started_at

    def cancel(self, sequence: int, command: int):
        with self._lock:
            self._requests.pop((sequence, command), None)


class FileListingState:
    def __init__(self):
        self._sequence = None
        self._entries = []
        self._chunk_count = 0
        self._wire_bytes = 0
        self._started_at = None
        self._first_packet_time = None
        self._last_packet_time = None
        self._lock = threading.Lock()

    def start(self, sequence: int, started_at: float) -> bool:
        with self._lock:
            if self._sequence is not None:
                return False

            self._sequence = sequence
            self._entries = []
            self._chunk_count = 0
            self._wire_bytes = 0
            self._started_at = started_at
            self._first_packet_time = None
            self._last_packet_time = None
            return True

    def is_expected(self, sequence: int) -> bool:
        with self._lock:
            return self._sequence == sequence

    def append(self, entries, packet_size: int, received_at: float):
        with self._lock:
            previous_packet_time = self._last_packet_time

            if self._first_packet_time is None:
                self._first_packet_time = received_at

            self._entries.extend(entries)
            self._chunk_count += 1
            self._wire_bytes += packet_size
            self._last_packet_time = received_at

            elapsed_from_request = received_at - self._started_at
            inter_chunk = None if previous_packet_time is None else received_at - previous_packet_time

            return len(self._entries), self._chunk_count, elapsed_from_request, inter_chunk

    def finish(self, received_at: float):
        with self._lock:
            if self._sequence is None or self._last_packet_time is None:
                return None

            result = {
                "sequence": self._sequence,
                "entries": self._entries,
                "entry_count": len(self._entries),
                "chunk_count": self._chunk_count,
                "wire_bytes": self._wire_bytes,
                "started_at": self._started_at,
                "first_response_latency": self._first_packet_time - self._started_at,
                "transfer_duration": received_at - self._started_at,
            }

            self._reset_locked()
            return result

    def cancel(self):
        with self._lock:
            sequence = self._sequence
            self._reset_locked()
            return sequence

    def _reset_locked(self):
        self._sequence = None
        self._entries = []
        self._chunk_count = 0
        self._wire_bytes = 0
        self._started_at = None
        self._first_packet_time = None
        self._last_packet_time = None


def receiver(
    ser: serial.Serial,
    stop_event: threading.Event,
    listing_state: FileListingState,
    request_metrics: RequestMetrics,
    baudrate: int,
    verbose: bool,
):
    stream_parser = PacketStreamParser()

    def print_completed_listing(completed):
        entries = completed["entries"]
        lines = []
        lines.append("")
        lines.append("FILE LISTING")
        lines.append("------------")

        if not entries:
            lines.append("<empty directory>")
        else:
            for index, entry in enumerate(entries, start=1):
                type_value = entry["type"]
                type_text = FILE_TYPES.get(type_value, f"?{type_value:02X}")
                size_text = "" if type_value == 1 else human_size(entry["size"])
                line = f"{index:3}. [{type_text}] {entry['path']}"

                if size_text:
                    line += f"  {size_text}"

                lines.append(line)

        lines.append("")
        lines.append(
            f"LIST_FILES complete: {completed['entry_count']} entries in "
            f"{completed['chunk_count']} packet(s)"
        )
        lines.append(
            f"[TIME] LIST_FILES first response : "
            f"{format_duration(completed['first_response_latency'])}"
        )
        lines.append(
            f"[TIME] LIST_FILES final duration : "
            f"{format_duration(completed['transfer_duration'])}"
        )

        if completed["transfer_duration"] > 0:
            throughput = completed["wire_bytes"] / completed["transfer_duration"]
            lines.append(
                f"[TIME] Received wire data       : {completed['wire_bytes']} bytes "
                f"({throughput / 1024:.1f} KiB/s)"
            )

        uart_floor = completed["wire_bytes"] * 10.0 / baudrate
        lines.append(
            f"[TIME] UART 8N1 wire floor      : {format_duration(uart_floor)} "
            f"at {baudrate} baud"
        )

        output_started_at = time.perf_counter()
        print("\n".join(lines), flush=True)
        output_finished_at = time.perf_counter()

        total_duration = output_finished_at - completed["started_at"]
        client_finish_duration = max(0.0, total_duration - completed["transfer_duration"])
        output_duration = output_finished_at - output_started_at

        print(
            f"[TIME] LIST_FILES client finish   : {format_duration(client_finish_duration)} "
            f"(format/output {format_duration(output_duration)})"
        )
        print(f"[TIME] LIST_FILES TOTAL duration  : {format_duration(total_duration)}")
        print("> ", end="", flush=True)

    while ser.is_open and not stop_event.is_set():
        try:
            waiting = ser.in_waiting

            if waiting <= 0:
                time.sleep(0.001)
                continue

            data = ser.read(waiting)

            if not data:
                continue

            packets = stream_parser.push(data)

            for packet_data in packets:
                received_at = time.perf_counter()
                packet = display_packet(packet_data, verbose=verbose)

                if packet is None:
                    continue

                command = packet["command"]
                sequence = packet["sequence"]

                if command == COMMAND_LIST_FILES:
                    if not listing_state.is_expected(sequence):
                        print(f"Ignoring unexpected LIST_FILES response with sequence {sequence}")
                        continue

                    result = decode_file_listing(packet["payload"])

                    if result is None:
                        listing_state.cancel()
                        duration = request_metrics.finish(sequence, command, received_at)

                        if duration is not None:
                            print(f"[TIME] LIST_FILES failed after {format_duration(duration)}")

                        print("> ", end="", flush=True)
                        continue

                    if result["entries"] is None:
                        print()
                        print("FILE LISTING")
                        print("------------")
                        print(f"Status : {status_name(result['status'])}")

                        listing_state.cancel()
                        duration = request_metrics.finish(sequence, command, received_at)

                        if duration is not None:
                            print(f"[TIME] LIST_FILES seq={sequence}: {format_duration(duration)}")

                        print("> ", end="", flush=True)
                        continue

                    entries = result["entries"]
                    total_entries, chunk_count, elapsed_from_request, inter_chunk = listing_state.append(
                        entries, len(packet_data), received_at
                    )

                    if verbose:
                        if inter_chunk is None:
                            print(
                                f"[TIME] LIST_FILES chunk {chunk_count}: "
                                f"{len(entries)} entries, {format_duration(elapsed_from_request)} from request"
                            )
                        else:
                            print(
                                f"[TIME] LIST_FILES chunk {chunk_count}: {len(entries)} entries, "
                                f"{format_duration(elapsed_from_request)} from request, "
                                f"+{format_duration(inter_chunk)} since previous chunk"
                            )

                    if result["status"] == STATUS_OK:
                        completed = listing_state.finish(received_at)
                        request_metrics.finish(sequence, command, received_at)

                        if completed is not None:
                            print_completed_listing(completed)
                    elif result["status"] != STATUS_NOT_FINISHED_YET:
                        listing_state.cancel()
                        request_metrics.finish(sequence, command, received_at)
                        print(f"Unexpected LIST_FILES status: {status_name(result['status'])}")
                        print("> ", end="", flush=True)
                else:
                    duration = request_metrics.finish(sequence, command, received_at)

                    if duration is not None:
                        command_name = COMMAND_NAMES.get(command, f"0x{command:02X}")
                        print(f"[TIME] {command_name} seq={sequence}: {format_duration(duration)}")

                    print("> ", end="", flush=True)
        except serial.SerialException:
            break


def send_packet(
    ser: serial.Serial,
    sequence: int,
    command: int,
    payload: bytes = b"",
    request_metrics: RequestMetrics = None,
    started_at: float = None,
    verbose: bool = False,
):
    if started_at is None:
        started_at = time.perf_counter()

    if request_metrics is not None:
        request_metrics.start(sequence, command, started_at)

    data = build_packet(sequence, command, payload)
    command_name = COMMAND_NAMES.get(command, f"0x{command:02X}")

    print(f"[TX] {command_name} seq={sequence} bytes={len(data)}")

    ser.write(data)
    ser.flush()

    if verbose:
        print(f"TX HEX : {data.hex(' ').upper()}")

    return started_at


def send_hex(ser: serial.Serial, value: str):
    value = value.replace(" ", "")

    try:
        data = bytes.fromhex(value)
        crc16 = calculate_crc16(data)
        data += crc16.to_bytes(2, byteorder="little")
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
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument("--port", required=True)
    argument_parser.add_argument("--baud", type=int, default=115200)
    argument_parser.add_argument("--verbose", action="store_true")
    args = argument_parser.parse_args()

    ser = serial.Serial(port=args.port, baudrate=args.baud, timeout=0.05)

    time.sleep(0.5)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print(f"Connected to {args.port} @ {args.baud}")
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

    sequence = SequenceCounter()
    listing_state = FileListingState()
    request_metrics = RequestMetrics()
    stop_event = threading.Event()

    thread = threading.Thread(
        target=receiver,
        args=(ser, stop_event, listing_state, request_metrics, args.baud, args.verbose),
        daemon=True,
    )
    thread.start()

    try:
        while True:
            value = input("> ").strip()

            if value == "/quit":
                break

            if value == "/clear":
                ser.reset_input_buffer()
                print("RX buffer cleared")
                continue

            if value == "/ping":
                send_packet(ser, sequence.next(), COMMAND_PING, request_metrics=request_metrics, verbose=args.verbose)
                continue

            if value == "/cart":
                send_packet(ser, sequence.next(), COMMAND_GET_CART_INFO, request_metrics=request_metrics, verbose=args.verbose)
                continue

            if value == "/ls":
                path = "/"
                payload = encode_string(path)
                request_sequence = sequence.next()
                started_at = time.perf_counter()

                if not listing_state.start(request_sequence, started_at):
                    print("A LIST_FILES request is already in progress")
                    continue

                send_packet(
                    ser,
                    request_sequence,
                    COMMAND_LIST_FILES,
                    payload,
                    request_metrics=request_metrics,
                    started_at=started_at,
                    verbose=args.verbose,
                )
                continue

            if value.startswith("/ls "):
                path = value[4:].strip()

                if not path:
                    path = "/"

                if not path.startswith("/"):
                    path = "/" + path

                payload = encode_string(path)
                request_sequence = sequence.next()
                started_at = time.perf_counter()

                if not listing_state.start(request_sequence, started_at):
                    print("A LIST_FILES request is already in progress")
                    continue

                send_packet(
                    ser,
                    request_sequence,
                    COMMAND_LIST_FILES,
                    payload,
                    request_metrics=request_metrics,
                    started_at=started_at,
                    verbose=args.verbose,
                )
                continue

            if value.startswith("/hex "):
                send_hex(ser, value[5:])
                continue

            send_text(ser, value)
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()

        if ser.is_open:
            ser.close()

        thread.join(timeout=0.5)

    print("Disconnected")


if __name__ == "__main__":
    main()
