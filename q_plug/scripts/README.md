# Validation scripts

Three scripts, one per plugin format, each running the standard validator
for that format against a built plugin. Run them from anywhere; each
resolves the repo root from its own location. Exit codes: 0 when every
validator that ran passed, 1 on any failure, and 77 when no validator was
installed to run. ctest treats 77 as SKIPPED; the top-level CMake registers
these scripts as tests, so `ctest --test-dir build` runs them all.

Which plugin they validate is set by environment variables, with defaults
for the gain example:

| Variable      | Default      | Meaning                                    |
|---------------|--------------|--------------------------------------------|
| `PLUGIN_NAME` | `QPlug Gain` | Bundle name, without extension             |
| `BUILD_DIR`   | `build`      | Directory whose `products/` holds bundles  |
| `AU_TYPE`     | `aufx`       | AU component type (`validate-au.sh` only)  |
| `AU_SUBTYPE`  | `QGan`       | AU subtype code                            |
| `AU_MFR`      | `QPlg`       | AU manufacturer code                       |
| `CLAP_VALIDATOR` | (search)  | Path to clap-validator                     |
| `PLUGINVAL`   | (search)     | Path to pluginval                          |

The top-level CMake sets the last two to the pinned validators it downloads
(see `cmake/qplug_validators.cmake`), so `ctest` needs no installs. When
running a script by hand without them set, the script searches PATH and the
usual install locations.

The AU codes must match what the plugin's CMake passes to clap-wrapper as
`AUV2_INSTRUMENT_TYPE`, `AUV2_SUBTYPE_CODE` and `AUV2_MANUFACTURER_CODE`.

Build first, so that `BUILD_DIR/products/` holds `NAME.clap`, `NAME.vst3`
and `NAME.component`. For example:

```
cd examples/gain
cmake -B ../../build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build ../../build
```

## validate.sh

Runs `clap-validator` on the CLAP bundle. A clean result is 0 failed; skips
for extensions the plugin does not implement are normal.

clap-validator is not installed by Homebrew. Download the macOS build from
github.com/free-audio/clap-validator/releases and either put it on PATH,
in `~/.cargo/bin` or `/usr/local/bin`, or point at it explicitly:

```
CLAP_VALIDATOR=/path/to/clap-validator ./scripts/validate.sh
```

## validate-vst3.sh

Runs pluginval at strictness level 5 on the VST3 bundle, in process.
Expected: SUCCESS. If the Steinberg SDK validator is found it runs that
too; otherwise it says so and skips.

pluginval is a cask (`brew install --cask pluginval`) and is not on PATH;
the script finds it at `/Applications/pluginval.app/Contents/MacOS/pluginval`.

## validate-au.sh

The script first checks that the bundle's Info.plist carries an
`AudioComponents` entry and refuses to install it otherwise. A plist without
one is what a reconfigure without a rebuild produces (see
`cmake/qplug_auv2_plist.cmake`); the fix is `cmake --build`.

auval only sees components installed in the system, so this script then
copies the bundle into `~/Library/Audio/Plug-Ins/Components` with `ditto`
(which replaces in place; `cp -r` would nest a bundle inside the old one),
waits for the system to register it, restarting `AudioComponentRegistrar`
at most once if it has not after 30 seconds (a restart forces a rescan of
every component on the machine, so it is never done repeatedly), then runs

```
auval -v $AU_TYPE $AU_SUBTYPE $AU_MFR
```

It also compares the component version the registry serves with the one the
installed bundle declares. They disagree when the registrar still holds an
older registration for the bundle, which no amount of copying or touching
clears; a host then reads the old description and offers the plugin with
its old channel layout, opening to an empty window. On a mismatch the
script restarts the registrar once, and warns if the two still disagree.

The script then runs pluginval on the same bundle. Expected: AU VALIDATION
SUCCEEDED and 2 passed. Three auval warnings are normal and come from clap-wrapper,
not from the plugin: Tail Time not supported, preset name not retained in
class data, and MusicDeviceMIDIEvent implemented on an effect type.

The side effect is real: after running this, the plugin is installed and
AU hosts such as Logic Pro will list it. Hosts cache what the registry told
them, keyed on the component version, so bump the plugin's version whenever
its shape changes. clap-wrapper derives that version from the bundle
version, and `0.01` and `0.1.0` both come out as 256: a version that
reads as new is not always new.
