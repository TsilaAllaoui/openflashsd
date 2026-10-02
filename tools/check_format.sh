#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

find_clang_format()
{
    if [[ -n "${CLANG_FORMAT:-}" ]]; then
        printf '%s\n' "$CLANG_FORMAT"
        return
    fi

    if command -v clang-format >/dev/null 2>&1; then
        command -v clang-format
        return
    fi

    for version in 19 18 17 16 15 14; do
        if command -v "clang-format-$version" >/dev/null 2>&1; then
            command -v "clang-format-$version"
            return
        fi
    done

    echo "clang-format not found. Install it with: sudo apt install clang-format" >&2
    exit 1
}

CLANG_FORMAT_BIN="$(find_clang_format)"

mapfile -d '' FILES < <(
    find \
        "$ROOT/protocol" \
        "$ROOT/gba/include" \
        "$ROOT/gba/src" \
        "$ROOT/esp32/openflash_esp32" \
        "$ROOT/esp32/sender" \
        "$ROOT/esp32/tests" \
        -type d \( -name generated -o -name build -o -name .git \) -prune -o \
        -type f \( \
            -name '*.c' -o \
            -name '*.cc' -o \
            -name '*.cpp' -o \
            -name '*.cxx' -o \
            -name '*.h' -o \
            -name '*.hh' -o \
            -name '*.hpp' -o \
            -name '*.hxx' -o \
            -name '*.ino' \
        \) -print0
)

if [[ ${#FILES[@]} -eq 0 ]]; then
    echo "No source files found."
    exit 0
fi

"$CLANG_FORMAT_BIN" --dry-run --Werror -style=file "${FILES[@]}"
echo "Formatting is clean for ${#FILES[@]} source files."
