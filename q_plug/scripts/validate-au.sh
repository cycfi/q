#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$SCRIPT_DIR/.."
BUILD="${BUILD_DIR:-$ROOT/build}"
PLUGIN_NAME="${PLUGIN_NAME:-QPlug Gain}"
PLUGIN="$BUILD/products/$PLUGIN_NAME.component"
AU_INSTALL="$HOME/Library/Audio/Plug-Ins/Components/$PLUGIN_NAME.component"

# Component type, subtype and manufacturer codes, as declared to clap-wrapper
AU_TYPE="${AU_TYPE:-aufx}"
AU_SUBTYPE="${AU_SUBTYPE:-QGan}"
AU_MFR="${AU_MFR:-QPlg}"

if [ ! -d "$PLUGIN" ]; then
    echo "ERROR: AUv2 not built. Run: cmake --build build"
    exit 1
fi

# auval requires the component to be registered (copied to Components folder).
# ditto overwrites the bundle in place; cp -r would nest it inside an
# existing one.
# Never install a bundle the system cannot register. clap-wrapper's plist
# merge runs only when the AUv2 target relinks, so a reconfigure without a
# rebuild leaves a plist with no AudioComponents (see
# cmake/qplug_auv2_plist.cmake).
if ! grep -q AudioComponents "$PLUGIN/Contents/Info.plist"; then
    echo "ERROR: $PLUGIN has no AudioComponents entry in its Info.plist."
    echo "  Run cmake --build first; a reconfigure alone leaves it broken."
    exit 1
fi

echo "Installing component for auval..."
ditto "$PLUGIN" "$AU_INSTALL"
# The system normally registers the component on its own once the bundle
# lands, so wait for that first. Restarting AudioComponentRegistrar forces a
# rescan of every component on the machine, which takes minutes and must
# never be done repeatedly, so it is used at most once, and only if the
# system has not picked the component up on its own.
run_auval()
{
    local tries=15 restarted=0 status out
    while :; do
        status=0
        out=$(auval -v "$AU_TYPE" "$AU_SUBTYPE" "$AU_MFR" 2>&1) || status=$?
        if echo "$out" | grep -q "didn't find the component"
        then
            if [ "$tries" -gt 0 ]; then
                echo "  waiting for the component to be registered..."
                sleep 2
                tries=$((tries-1))
                continue
            fi
            if [ "$restarted" -eq 0 ]; then
                echo "  not registered after 30s; restarting the registrar once"
                killall -9 AudioComponentRegistrar 2>/dev/null || true
                restarted=1
                tries=90
                continue
            fi
        fi
        echo "$out"
        return "$status"
    done
}

PASS=0
FAIL=0

# ------------------------------------------------------------------
# auval  (type/subtype/mfr from AUV2_INSTRUMENT_TYPE / SUBTYPE / MFR)
# ------------------------------------------------------------------
if command -v auval &>/dev/null; then
    echo "=== auval ==="
    if run_auval
    then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
    fi
else
    echo "SKIP: auval not found (macOS only)"
fi

# ------------------------------------------------------------------
# pluginval
# ------------------------------------------------------------------
PLUGINVAL_BIN=""
for candidate in \
    "${PLUGINVAL:-}" \
    "pluginval" \
    "/Applications/pluginval.app/Contents/MacOS/pluginval"; do
    [ -z "$candidate" ] && continue
    if command -v "$candidate" &>/dev/null || [ -x "$candidate" ]; then
        PLUGINVAL_BIN="$candidate"
        break
    fi
done

if [ -n "$PLUGINVAL_BIN" ]; then
    echo "=== pluginval (AU) ==="
    if "$PLUGINVAL_BIN" --validate-in-process --strictness-level 5 "$PLUGIN"
    then
        PASS=$((PASS+1))
    else
        FAIL=$((FAIL+1))
    fi
else
    echo "SKIP: pluginval not found"
    echo "  brew install --cask pluginval"
    echo "  or https://github.com/Tracktion/pluginval"
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
if [ $((PASS + FAIL)) -eq 0 ]; then
    echo "SKIP: no validator ran"
    exit 77
fi
[ "$FAIL" -eq 0 ]
