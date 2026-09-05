#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
PLUGIN="$ROOT/build/products/QPlug Gain.component"
AU_INSTALL="$HOME/Library/Audio/Plug-Ins/Components/QPlug Gain.component"

if [ ! -d "$PLUGIN" ]; then
    echo "ERROR: AUv2 not built. Run: cmake --build build"
    exit 1
fi

# auval requires the component to be registered (copied to Components folder)
echo "Installing component for auval..."
cp -r "$PLUGIN" "$AU_INSTALL"
# Kick the AU cache
killall -9 AudioComponentRegistrar 2>/dev/null || true

PASS=0
FAIL=0

# ------------------------------------------------------------------
# auval  (type/subtype/mfr from AUV2_INSTRUMENT_TYPE / SUBTYPE / MFR)
# ------------------------------------------------------------------
if command -v auval &>/dev/null; then
    echo "=== auval ==="
    # aufx QGan QPlg
    auval -v aufx QGan QPlg && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
else
    echo "SKIP: auval not found (macOS only)"
fi

# ------------------------------------------------------------------
# pluginval
# ------------------------------------------------------------------
PLUGINVAL_BIN=""
for candidate in \
    "pluginval" \
    "/Applications/pluginval.app/Contents/MacOS/pluginval"; do
    if command -v "$candidate" &>/dev/null || [ -x "$candidate" ]; then
        PLUGINVAL_BIN="$candidate"
        break
    fi
done

if [ -n "$PLUGINVAL_BIN" ]; then
    echo "=== pluginval (AU) ==="
    "$PLUGINVAL_BIN" --validate-in-process --strictness-level 5 "$PLUGIN" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
else
    echo "SKIP: pluginval not found (brew install pluginval or https://github.com/Tracktion/pluginval)"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
