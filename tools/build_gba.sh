#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GBA_DIR="$ROOT_DIR/gba"

JOBS="${JOBS:-$(nproc)}"

OPENFLASH_MOCK="${OPENFLASH_MOCK:-1}"
OPENFLASH_DEBUG="${OPENFLASH_DEBUG:-0}"

GENERATE_MAPS=0
RUN_EMULATOR=0

usage()
{
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  --maps      Regenerate C++ map files from TMX files before building"
    echo "  --run       Run the generated ROM with mgba-qt after building"
    echo "  --help      Show this help"
    echo "  --clean     Clean the build directory"
    echo
    echo "Environment:"
    echo "  JOBS=<n>"
    echo "  OPENFLASH_MOCK=0|1"
    echo "  OPENFLASH_DEBUG=0|1"
}

while [ $# -gt 0 ]
do
    case "$1" in
        --maps)
            GENERATE_MAPS=1
            ;;

        --run)
            RUN_EMULATOR=1
            ;;

        --help|-h)
            usage
            exit 0
            ;;

        --clean)
            echo "Cleaning build directory..."
            make -C "$GBA_DIR" clean
            ;;

        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac

    shift
done

if [ "$GENERATE_MAPS" -eq 1 ]
then
    echo "Generating GBA maps..."

    (
        cd "$GBA_DIR"
        python3 tools/tmx_to_cpp.py maps
    )
fi

echo "Building GBA..."
echo "  OPENFLASH_MOCK=$OPENFLASH_MOCK"
echo "  OPENFLASH_DEBUG=$OPENFLASH_DEBUG"
echo "  JOBS=$JOBS"

make \
    -C "$GBA_DIR" \
    -j"$JOBS" \
    OPENFLASH_MOCK="$OPENFLASH_MOCK" \
    OPENFLASH_DEBUG="$OPENFLASH_DEBUG"

ROM="$GBA_DIR/gba.gba"

if [ "$RUN_EMULATOR" -eq 1 ]
then
    echo "Starting mGBA..."
    mgba-qt "$ROM"
fi