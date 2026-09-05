#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
PLUGIN="$ROOT/build/products/QPlug Gain.vst3"

if [ ! -d "$PLUGIN" ]; then
    echo "ERROR: VST3 not built. Run: cmake --build build"
    exit 1
fi

PASS=0
FAIL=0

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
    echo "=== pluginval (VST3) ==="
    "$PLUGINVAL_BIN" --validate-in-process --strictness-level 5 "$PLUGIN" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
else
    echo "SKIP: pluginval not found (brew install pluginval or https://github.com/Tracktion/pluginval)"
fi

# ------------------------------------------------------------------
# Steinberg VST3 SDK validator
# ------------------------------------------------------------------
STEINBERG_VALIDATOR=""
for candidate in \
    "$(xcode-select -p 2>/dev/null)/../SharedFrameworks/vst3sdk/bin/validator" \
    "/usr/local/bin/vstvalidator" \
    "vstvalidator"; do
    if [ -x "$candidate" ] 2>/dev/null || command -v "$candidate" &>/dev/null; then
        STEINBERG_VALIDATOR="$candidate"
        break
    fi
done

if [ -n "$STEINBERG_VALIDATOR" ]; then
    echo "=== Steinberg VST3 validator ==="
    "$STEINBERG_VALIDATOR" "$PLUGIN" && PASS=$((PASS+1)) || FAIL=$((FAIL+1))
else
    echo "SKIP: Steinberg vst3 validator not found"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
