--------------------------------------------------
-- Template for OpenFlash mGBA mailbox server
--
-- This script emulates the ESP32 side of OpenFlash enough
-- to test the GBA client without real ESP32 hardware.
--
-- build_gba.sh replaces the MAILBOX address automatically
-- when OPENFLASH_LUA=1.
--------------------------------------------------

local MAILBOX =
    0x02000000 -- CHANGE THIS


--------------------------------------------------
-- Mailbox layout
--------------------------------------------------

local TX_READ =
    MAILBOX + 0

local TX_WRITE =
    MAILBOX + 2

local RX_READ =
    MAILBOX + 4

local RX_WRITE =
    MAILBOX + 6

local TX_OVERFLOW =
    MAILBOX + 8

local RX_OVERFLOW =
    MAILBOX + 9

local TX_DATA =
    MAILBOX + 12

local RX_DATA =
    MAILBOX + 8204


--------------------------------------------------
-- Ring configuration
--------------------------------------------------

local BUFFER_SIZE =
    8192

local BUFFER_MASK =
    BUFFER_SIZE - 1


--------------------------------------------------
-- OpenFlash protocol
--------------------------------------------------

local MAGIC_0 =
    0x4F

local MAGIC_1 =
    0x46

local PROTOCOL_VERSION =
    1

local CMD_PING =
    0x01

local CMD_GET_CART_INFO =
    0x02

local CMD_LIST_FILES =
    0x03

local CMD_DEBUG =
    0x09

local STATUS_OK =
    0x00

local STATUS_ERROR =
    0x01

local STATUS_INVALID_COMMAND =
    0x02

local STATUS_INVALID_PAYLOAD =
    0x03

local STATUS_NOT_FOUND =
    0x04

local STATUS_NOT_READY =
    0x05

local STATUS_NOT_FINISHED_YET =
    0x06

local MAX_PAYLOAD_SIZE =
    4096


--------------------------------------------------
-- File entry protocol
--------------------------------------------------

local FILE_TYPE_NORMAL =
    0

local FILE_TYPE_FOLDER =
    1

local FILE_TYPE_GBA =
    2

local FILE_TYPE_SAVE =
    3

local TYPE_SIZE =
    1

local FILE_SIZE_SIZE =
    4

local FILE_PATH_LENGTH_SIZE =
    2

local FILE_ENTRY_MINIMAL_SIZE =
    TYPE_SIZE +
    FILE_SIZE_SIZE +
    FILE_PATH_LENGTH_SIZE

local FILE_LIST_HEADER_SIZE =
    5


--------------------------------------------------
-- Mock filesystem
--
-- Edit this table whenever you want different test files.
--------------------------------------------------

local MOCK_FILES =
{
    ["/"] =
    {
        {
            type = FILE_TYPE_FOLDER,
            size = 0,
            path = "/GAMES"
        },
        {
            type = FILE_TYPE_FOLDER,
            size = 0,
            path = "/SAVES"
        },
        {
            type = FILE_TYPE_FOLDER,
            size = 0,
            path = "/gba"
        }
    },

    ["/GAMES"] =
    {
        {
            type = FILE_TYPE_GBA,
            size = 16777216,
            path = "/GAMES/PokemonEmerald.gba"
        },
        {
            type = FILE_TYPE_GBA,
            size = 16777216,
            path = "/GAMES/PokemonRuby.gba"
        },
        {
            type = FILE_TYPE_GBA,
            size = 16777216,
            path = "/GAMES/PokemonSapphire.gba"
        },
        {
            type = FILE_TYPE_GBA,
            size = 33554432,
            path = "/GAMES/DragonBallAdvancedAdventure.gba"
        },
        {
            type = FILE_TYPE_GBA,
            size = 16777216,
            path = "/GAMES/WarioLand4.gba"
        }
    },

    ["/SAVES"] =
    {
        {
            type = FILE_TYPE_SAVE,
            size = 131072,
            path = "/SAVES/PokemonEmerald.sav"
        },
        {
            type = FILE_TYPE_SAVE,
            size = 131072,
            path = "/SAVES/PokemonRuby.sav"
        },
        {
            type = FILE_TYPE_SAVE,
            size = 131072,
            path = "/SAVES/PokemonSapphire.sav"
        }
    },

    ["/gba"] =
    {
        {
            type = FILE_TYPE_NORMAL,
            size = 1234,
            path = "/gba/readme.txt"
        },
        {
            type = FILE_TYPE_FOLDER,
            size = 0,
            path = "/gba/test"
        }
    },

    ["/gba/test"] =
    {
        {
            type = FILE_TYPE_NORMAL,
            size = 42,
            path = "/gba/test/example.txt"
        }
    }
}


--------------------------------------------------
-- Server state
--------------------------------------------------

local tx_packet_buffer =
    {}

local last_ping_sequence =
    nil

local AUTO_PONG =
    true

-- Used only for packets manually initiated by this Lua server.
local server_sequence_number =
    0

-- F6 edge detection.
local f6_was_down =
    false


--------------------------------------------------
-- Logging
--------------------------------------------------

local function log(text)

    console:log(
        "[OpenFlash] " .. text
    )

end


local function warn(text)

    console:warn(
        "[OpenFlash] " .. text
    )

end


--------------------------------------------------
-- Formatting
--------------------------------------------------

local function hex_byte(value)

    return string.format(
        "%02X",
        value & 0xFF
    )

end


local function hex_bytes(bytes)

    local parts =
        {}

    for i = 1, #bytes do

        parts[#parts + 1] =
            hex_byte(
                bytes[i]
            )

    end

    return table.concat(
        parts,
        " "
    )

end


local function payload_text(
    payload
)

    local result =
        {}

    for i = 1, #payload do

        local value =
            payload[i]

        if
            value >= 32 and
            value <= 126
        then

            result[#result + 1] =
                string.char(value)

        else

            result[#result + 1] =
                string.format(
                    "\\x%02X",
                    value
                )

        end

    end

    return table.concat(result)

end


--------------------------------------------------
-- Payload utilities
--------------------------------------------------

local function append_u8(
    bytes,
    value
)

    bytes[#bytes + 1] =
        value & 0xFF

end


local function append_u16(
    bytes,
    value
)

    bytes[#bytes + 1] =
        value & 0xFF

    bytes[#bytes + 1] =
        (value >> 8) & 0xFF

end


local function append_u32(
    bytes,
    value
)

    bytes[#bytes + 1] =
        value & 0xFF

    bytes[#bytes + 1] =
        (value >> 8) & 0xFF

    bytes[#bytes + 1] =
        (value >> 16) & 0xFF

    bytes[#bytes + 1] =
        (value >> 24) & 0xFF

end


local function append_string(
    bytes,
    value
)

    append_u16(
        bytes,
        #value
    )

    for i = 1, #value do

        bytes[#bytes + 1] =
            string.byte(
                value,
                i
            )

    end

end


--------------------------------------------------
-- Read LIST_FILES request path
--
-- Current GBA code sends the path as raw bytes:
--
--     "/"       -> 2F
--     "/GAMES"  -> 2F 47 41 4D 45 53
--
-- The ESP32 request_handler example uses read_string(),
-- which may use a 16-bit length-prefixed representation:
--
--     "/GAMES"  -> 06 00 2F 47 41 4D 45 53
--
-- Accept both so the mock remains compatible with either side.
--------------------------------------------------

local function read_request_path(
    payload
)

    if
        #payload == 0
    then

        return nil

    end

    --------------------------------------------------
    -- Try 16-bit length-prefixed string first.
    --------------------------------------------------

    if
        #payload >= 2
    then

        local declared_length =
            payload[1] |
            (payload[2] << 8)

        if
            declared_length ==
            #payload - 2
        then

            local characters =
                {}

            for i = 1, declared_length do

                characters[#characters + 1] =
                    string.char(
                        payload[2 + i]
                    )

            end

            return table.concat(
                characters
            )

        end

    end

    --------------------------------------------------
    -- Otherwise treat the whole payload as raw path bytes.
    --------------------------------------------------

    local characters =
        {}

    for i = 1, #payload do

        local value =
            payload[i]

        if
            value == 0
        then

            break

        end

        characters[#characters + 1] =
            string.char(
                value
            )

    end

    if
        #characters == 0
    then

        return nil

    end

    return table.concat(
        characters
    )

end


local function normalize_path(
    path
)

    if
        path == nil or
        path == ""
    then

        return "/"

    end

    if
        #path > 1 and
        string.sub(path, -1) == "/"
    then

        return string.sub(
            path,
            1,
            #path - 1
        )

    end

    return path

end


--------------------------------------------------
-- CRC16
--------------------------------------------------

local function crc16(
    bytes,
    count
)

    local crc =
        0xFFFF

    local size =
        count or #bytes

    for i = 1, size do

        crc =
            crc ~
            ((bytes[i] & 0xFF) << 8)

        for _ = 1, 8 do

            if
                (crc & 0x8000) ~= 0
            then

                crc =
                    ((crc << 1) ~ 0x1021)
                    & 0xFFFF

            else

                crc =
                    (crc << 1)
                    & 0xFFFF

            end

        end

    end

    return crc

end


--------------------------------------------------
-- Build OpenFlash packet
--------------------------------------------------

local function build_packet(
    sequence,
    command,
    payload
)

    payload =
        payload or {}

    local packet =
    {
        MAGIC_0,
        MAGIC_1,

        PROTOCOL_VERSION,

        sequence & 0xFF,

        command & 0xFF,

        #payload & 0xFF,
        (#payload >> 8) & 0xFF
    }

    for i = 1, #payload do

        packet[#packet + 1] =
            payload[i] & 0xFF

    end

    local crc =
        crc16(packet)

    packet[#packet + 1] =
        crc & 0xFF

    packet[#packet + 1] =
        (crc >> 8) & 0xFF

    return packet

end


--------------------------------------------------
-- Write bytes into GBA RX ring
--------------------------------------------------

local function send_to_gba(
    bytes
)

    local read =
        emu:read16(
            RX_READ
        )

    local write =
        emu:read16(
            RX_WRITE
        )

    for i = 1, #bytes do

        local next_write =
            (write + 1) &
            BUFFER_MASK

        if
            next_write == read
        then

            emu:write8(
                RX_OVERFLOW,
                1
            )

            warn(
                "RX mailbox full"
            )

            return false

        end

        emu:write8(
            RX_DATA + write,
            bytes[i]
        )

        write =
            next_write

    end

    --------------------------------------------------
    -- Publish after all packet bytes have been written.
    --------------------------------------------------

    emu:write16(
        RX_WRITE,
        write
    )

    log(
        "Peer -> GBA: " ..
        hex_bytes(bytes)
    )

    return true

end


--------------------------------------------------
-- Send a protocol response to the GBA
--------------------------------------------------

local function send_response(
    sequence,
    command,
    payload
)

    local packet =
        build_packet(
            sequence,
            command,
            payload
        )

    return send_to_gba(
        packet
    )

end


--------------------------------------------------
-- Send simple status response
--------------------------------------------------

local function send_status_response(
    sequence,
    command,
    status
)

    local payload =
        {}

    append_u8(
        payload,
        status
    )

    send_response(
        sequence,
        command,
        payload
    )

end


--------------------------------------------------
-- PING response
--------------------------------------------------

local function send_pong(
    sequence
)

    log(
        "Sending PONG sequence " ..
        sequence
    )

    send_status_response(
        sequence,
        CMD_PING,
        STATUS_OK
    )

end


--------------------------------------------------
-- F6 manual PING
--------------------------------------------------

local function send_manual_ping()

    local sequence =
        server_sequence_number

    local packet =
        build_packet(
            sequence,
            CMD_PING,
            {}
        )

    log(
        "F6 pressed - sending PING sequence " ..
        sequence
    )

    if send_to_gba(packet) then

        server_sequence_number =
            (server_sequence_number + 1) & 0xFF

    end

end


--------------------------------------------------
-- Emulator keyboard shortcuts
--------------------------------------------------

local function handle_shortcuts()

    local f6_down =
        input:isKeyActive(
            C.KEY.F6
        )

    if
        f6_down and
        not f6_was_down
    then

        send_manual_ping()

    end

    f6_was_down =
        f6_down

end


--------------------------------------------------
-- LIST_FILES response helpers
--------------------------------------------------

local function append_file_entry(
    payload,
    file
)

    append_u8(
        payload,
        file.type
    )

    append_u32(
        payload,
        file.size
    )

    append_string(
        payload,
        file.path
    )

end


local function build_file_list_payload(
    status,
    entries_payload,
    entries_size,
    file_count
)

    local payload =
        {}

    append_u8(
        payload,
        status
    )

    append_u16(
        payload,
        entries_size
    )

    append_u16(
        payload,
        file_count
    )

    for i = 1, #entries_payload do

        payload[#payload + 1] =
            entries_payload[i]

    end

    return payload

end


--------------------------------------------------
-- Handle LIST_FILES
--------------------------------------------------

local function handle_list_files(
    sequence,
    payload
)

    local request_path =
        read_request_path(
            payload
        )

    if request_path == nil then

        warn(
            "LIST_FILES invalid payload"
        )

        send_status_response(
            sequence,
            CMD_LIST_FILES,
            STATUS_INVALID_PAYLOAD
        )

        return

    end

    request_path =
        normalize_path(
            request_path
        )

    log(
        "LIST_FILES path=" ..
        request_path
    )

    local files =
        MOCK_FILES[
            request_path
        ]

    if files == nil then

        log(
            "LIST_FILES not found: " ..
            request_path
        )

        send_status_response(
            sequence,
            CMD_LIST_FILES,
            STATUS_NOT_FOUND
        )

        return

    end

    local current_entries =
        {}

    local file_count =
        0

    local estimated_size =
        0

    for i = 1, #files do

        local file =
            files[i]

        local current_entry_size =
            FILE_ENTRY_MINIMAL_SIZE +
            #file.path

        --------------------------------------------------
        -- Flush current packet before adding this entry
        -- if it would exceed the protocol payload limit.
        --------------------------------------------------

        if
            file_count > 0 and
            FILE_LIST_HEADER_SIZE +
            estimated_size +
            current_entry_size >
            MAX_PAYLOAD_SIZE
        then

            local response_payload =
                build_file_list_payload(
                    STATUS_NOT_FINISHED_YET,
                    current_entries,
                    estimated_size,
                    file_count
                )

            log(
                string.format(
                    "LIST_FILES chunk files=%d bytes=%d status=NOT_FINISHED_YET",
                    file_count,
                    estimated_size
                )
            )

            send_response(
                sequence,
                CMD_LIST_FILES,
                response_payload
            )

            current_entries =
                {}

            file_count =
                0

            estimated_size =
                0

        end

        append_file_entry(
            current_entries,
            file
        )

        estimated_size =
            estimated_size +
            current_entry_size

        file_count =
            file_count + 1

    end

    --------------------------------------------------
    -- Final chunk
    --------------------------------------------------

    local response_payload =
        build_file_list_payload(
            STATUS_OK,
            current_entries,
            estimated_size,
            file_count
        )

    log(
        string.format(
            "LIST_FILES final files=%d bytes=%d status=OK",
            file_count,
            estimated_size
        )
    )

    send_response(
        sequence,
        CMD_LIST_FILES,
        response_payload
    )

end


--------------------------------------------------
-- Handle packet received from GBA
--------------------------------------------------

local function handle_packet(
    packet
)

    local sequence =
        packet[4]

    local command =
        packet[5]

    local payload_size =
        packet[6] |
        (packet[7] << 8)

    local payload =
        {}

    for i = 1, payload_size do

        payload[#payload + 1] =
            packet[7 + i]

    end

    --------------------------------------------------
    -- DEBUG
    --------------------------------------------------

    if
        command == CMD_DEBUG
    then

        log(
            "GBA DEBUG: " ..
            payload_text(payload)
        )

        return

    end

    --------------------------------------------------
    -- Normal packet logging
    --------------------------------------------------

    log(
        string.format(
            "GBA -> server seq=%d cmd=0x%02X payload=%d",
            sequence,
            command,
            payload_size
        )
    )

    log(
        hex_bytes(packet)
    )

    --------------------------------------------------
    -- PING
    --------------------------------------------------

    if
        command == CMD_PING
    then

        last_ping_sequence =
            sequence

        if
            #payload ~= 0
        then

            warn(
                "PING invalid payload"
            )

            send_status_response(
                sequence,
                CMD_PING,
                STATUS_INVALID_PAYLOAD
            )

        elseif AUTO_PONG then

            log(
                "PING received"
            )

            send_pong(
                sequence
            )

        end

        return

    end

    --------------------------------------------------
    -- LIST_FILES
    --------------------------------------------------

    if
        command == CMD_LIST_FILES
    then

        handle_list_files(
            sequence,
            payload
        )

        return

    end

    --------------------------------------------------
    -- GET_CART_INFO
    --------------------------------------------------

    if
        command == CMD_GET_CART_INFO
    then

        warn(
            "GET_CART_INFO mock is not implemented yet"
        )

        send_status_response(
            sequence,
            CMD_GET_CART_INFO,
            STATUS_INVALID_COMMAND
        )

        return

    end

    --------------------------------------------------
    -- Unsupported command
    --------------------------------------------------

    warn(
        string.format(
            "Unhandled command 0x%02X",
            command
        )
    )

    send_status_response(
        sequence,
        command,
        STATUS_INVALID_COMMAND
    )

end


--------------------------------------------------
-- Validate packet
--------------------------------------------------

local function packet_valid(
    packet
)

    if
        #packet < 9
    then

        return false

    end

    if
        packet[1] ~= MAGIC_0 or
        packet[2] ~= MAGIC_1
    then

        return false

    end

    if
        packet[3] ~= PROTOCOL_VERSION
    then

        return false

    end

    local expected_crc =
        crc16(
            packet,
            #packet - 2
        )

    local received_crc =
        packet[#packet - 1] |
        (packet[#packet] << 8)

    return
        expected_crc ==
        received_crc

end


--------------------------------------------------
-- Process TX stream from GBA
--------------------------------------------------

local function process_tx_buffer()

    while true do

        --------------------------------------------------
        -- Find magic
        --------------------------------------------------

        while
            #tx_packet_buffer > 0 and
            tx_packet_buffer[1] ~= MAGIC_0
        do

            table.remove(
                tx_packet_buffer,
                1
            )

        end

        if
            #tx_packet_buffer < 2
        then

            return

        end

        if
            tx_packet_buffer[2] ~= MAGIC_1
        then

            table.remove(
                tx_packet_buffer,
                1
            )

            goto continue

        end

        --------------------------------------------------
        -- Need header
        --------------------------------------------------

        if
            #tx_packet_buffer < 7
        then

            return

        end

        local payload_size =
            tx_packet_buffer[6] |
            (tx_packet_buffer[7] << 8)

        if
            payload_size >
            MAX_PAYLOAD_SIZE
        then

            warn(
                "Invalid payload size " ..
                payload_size
            )

            table.remove(
                tx_packet_buffer,
                1
            )

            goto continue

        end

        local packet_size =
            9 +
            payload_size

        if
            #tx_packet_buffer <
            packet_size
        then

            return

        end

        --------------------------------------------------
        -- Extract packet
        --------------------------------------------------

        local packet =
            {}

        for i = 1, packet_size do

            packet[i] =
                tx_packet_buffer[i]

        end

        for _ = 1, packet_size do

            table.remove(
                tx_packet_buffer,
                1
            )

        end

        --------------------------------------------------
        -- Validate and handle
        --------------------------------------------------

        if packet_valid(packet) then

            handle_packet(
                packet
            )

        else

            warn(
                "Invalid packet or CRC: " ..
                hex_bytes(packet)
            )

        end

        ::continue::

    end

end


--------------------------------------------------
-- Drain bytes sent by GBA
--------------------------------------------------

local function drain_gba_tx()

    local read =
        emu:read16(
            TX_READ
        )

    local write =
        emu:read16(
            TX_WRITE
        )

    while
        read ~= write
    do

        local byte =
            emu:read8(
                TX_DATA + read
            )

        tx_packet_buffer[
            #tx_packet_buffer + 1
        ] =
            byte

        read =
            (read + 1) &
            BUFFER_MASK

    end

    --------------------------------------------------
    -- Tell GBA everything was consumed
    --------------------------------------------------

    emu:write16(
        TX_READ,
        read
    )

    process_tx_buffer()

end


--------------------------------------------------
-- Overflow diagnostics
--------------------------------------------------

local function check_overflow()

    if
        emu:read8(
            TX_OVERFLOW
        ) ~= 0
    then

        warn(
            "GBA TX mailbox overflow"
        )

        emu:write8(
            TX_OVERFLOW,
            0
        )

    end

    if
        emu:read8(
            RX_OVERFLOW
        ) ~= 0
    then

        warn(
            "GBA RX mailbox overflow"
        )

        emu:write8(
            RX_OVERFLOW,
            0
        )

    end

end


--------------------------------------------------
-- Frame callback
--------------------------------------------------

local function on_frame()

    handle_shortcuts()

    drain_gba_tx()

    check_overflow()

end


--------------------------------------------------
-- Startup
--------------------------------------------------

log(
    string.format(
        "Mailbox address = 0x%08X",
        MAILBOX
    )
)

log(
    "Virtual ESP32 mock server started"
)

log(
    "Mock filesystem enabled"
)

log(
    "Press F6 to send a PING packet to the GBA"
)

callbacks:add(
    "frame",
    on_frame
)
