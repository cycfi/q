#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD="$ROOT/build"
PLUGIN="$BUILD/products/QPlug Gain.clap"

# Try to find clap-validator. Set CLAP_VALIDATOR to override.
VALIDATOR=""
for candidate in \
    "${CLAP_VALIDATOR:-}" \
    "clap-validator" \
    "$HOME/.cargo/bin/clap-validator" \
    "/usr/local/bin/clap-validator"; do
    [ -z "$candidate" ] && continue
    if command -v "$candidate" &>/dev/null || [ -x "$candidate" ]; then
        VALIDATOR="$candidate"
        break
    fi
done

if [ -z "$VALIDATOR" ]; then
    echo "ERROR: clap-validator not found. Download from:"
    echo "  https://github.com/free-audio/clap-validator/releases"
    exit 1
fi

if [ ! -d "$PLUGIN" ]; then
    echo "Plugin not built. Run: cmake --build build"
    exit 1
fi

exec "$VALIDATOR" validate "$PLUGIN"
