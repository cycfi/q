#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD="${BUILD_DIR:-$ROOT/cmake-build-debug}"
PLUGIN_NAME="${PLUGIN_NAME:-QPlug Gain}"
PLUGIN="$BUILD/products/$PLUGIN_NAME.clap"
# clap-wrapper gives each format a folder of its own on Windows.
[ -e "$PLUGIN" ] || PLUGIN="$BUILD/products/CLAP/$PLUGIN_NAME.clap"

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
    echo "SKIP: clap-validator not found. Download from:"
    echo "  https://github.com/free-audio/clap-validator/releases"
    exit 77
fi

# A bundle directory on macOS, a plain file elsewhere.
if [ ! -e "$PLUGIN" ]; then
    echo "Plugin not built. Run: cmake --build build"
    exit 1
fi

exec "$VALIDATOR" validate "$PLUGIN"
