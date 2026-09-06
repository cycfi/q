# QPlug

> **Draft.** QPlug is in early development on the `clap_2026` branch. The
> API changes without notice and nothing here is production ready yet.

## Introduction

QPlug is a C++ framework for writing audio plugins. You write the DSP, the
parameter logic and the user interface as three plain C++ classes; QPlug
turns them into a native [CLAP](https://cleveraudio.org) plugin and, through
[clap-wrapper](https://github.com/free-audio/clap-wrapper), into VST3 and
AudioUnit v2 as well, from the one implementation.

QPlug is built on two other Cycfi libraries. [Q](https://github.com/cycfi/q)
provides the DSP and the audio-stream vocabulary the processor speaks.
[Elements](https://github.com/cycfi/elements) provides the GUI, and its
model interface is what links controls to parameters.

Plugin code contains no CLAP, VST3 or AudioUnit types. The only translation
unit that knows a plugin format is QPlug's own adapter.

The library is Open Source and released under the
[MIT License](https://opensource.org/licenses/MIT).

## Overview

A QPlug plugin is composed of three parts, each an ordinary class you write:

1. **processor**: the DSP. Runs on the audio thread. Reads the controller's
   parameters and processes audio in `process(in, out)`.

2. **controller**: the parameters and the logic that relates them, such as
   enabling a group of controls, units, tapering, and wiring between
   controls. Runs on the main thread. It is the hub the other two attach to
   and is unaware of both.

3. **presenter**: builds the user interface and links it to the controller's
   parameters. Owns the view, which exists only while the host has an
   editor open.

The processor and the presenter never see each other. Underneath sits
`base_plugin`, the host adapter, whose header is format-free and whose
implementation lives in one file per format. Today there is one: CLAP.

### Dependencies

All dependencies are git submodules under `lib/`:

- [q](https://github.com/cycfi/q): DSP
- [elements](https://github.com/cycfi/elements): GUI
- [infra](https://github.com/cycfi/infra): shared utilities
- [clap](https://github.com/free-audio/clap): the CLAP headers
- [clap-wrapper](https://github.com/free-audio/clap-wrapper): VST3 and AUv2

The VST3 and AudioUnit SDKs are downloaded by clap-wrapper at configure
time. C++20 is required.

## Building

### Prerequisites

macOS is the only platform exercised so far. On a fresh Mac you need three
things; everything else is fetched by CMake at configure time.

- **Xcode Command Line Tools**, `xcode-select --install`. Provides the
  C++20 compiler, the macOS SDK and frameworks, and git. Xcode 14 or later.
- **CMake 3.21 or later.** Ninja as well if you use the presets below;
  plain `cmake -B build` works with CMake alone.
- **Network access at configure time.** CMake fetches any missing
  submodules, clap-wrapper downloads the VST3 SDK and Apple's AudioUnitSDK,
  and the validators the tests need are downloaded, pinned and checksummed,
  into `~/.cache/cycfi/qplug-validators`. Building the VST3 means accepting
  Steinberg's VST3 SDK license, GPLv3 or the proprietary agreement; see
  clap-wrapper's README.

auval ships with macOS. To use your own validator installs instead of the
downloaded ones, pass `-DCLAP_VALIDATOR=...` and `-DPLUGINVAL=...`, or set
`-DQPLUG_DOWNLOAD_VALIDATORS=OFF` to have CMake look for installed copies. A
validator that cannot be found makes its test skip, not fail. See
[scripts/README.md](scripts/README.md).

Elements' graphics toolchain (Skia or Cairo) is not needed yet. There is no
GUI code in the build so far; Elements is a submodule but is not compiled.

### Build and test

```
git clone https://github.com/cycfi/qplug.git
cd qplug
cmake --preset default
cmake --build build
ctest --test-dir build
```

Without Ninja, replace the preset line with
`cmake -B build -DCMAKE_BUILD_TYPE=Debug`. A `release` preset builds into
`build-release`.

Always run `cmake --build` before `ctest`. A reconfigure on its own leaves
the AUv2 bundle without its `AudioComponents` entry until the next build, a
clap-wrapper quirk that QPlug works around at build time; the AU test
refuses to install such a bundle and says so.

This builds every example into `build/products/` in all three formats and
runs the format validators on them. Note that the AU test installs the
component into `~/Library/Audio/Plug-Ins/Components` so that auval, and
any AU host, can see it.

The examples live under `examples/`. `gain` is the smallest: one stereo
gain with a single automatable parameter.

## Documentation

Not yet. The code is the record until the API settles.

## <a name="jdeguzman"></a>About the Author

Joel got into electronics and programming in the 80s because almost
everything in music, his first love, is becoming electronic and digital.
Since then, he builds his own guitars, effect boxes and synths. He enjoys
playing distortion-laden rock guitar, composes and produces his own music in
his home studio.

Joel de Guzman is the principal architect and engineer at [Cycfi
Research][1]. He is a software engineer specializing in advanced C++ and an
advocate of Open Source. He has authored a number of highly successful Open
Source projects such as [Boost.Spirit][3], [Boost.Phoenix][4] and
[Boost.Fusion][5]. These libraries are all part of the [Boost Libraries][6],
a well respected, peer-reviewed, Open Source, collaborative development
effort.

[1]: https://www.cycfi.com/
[3]: http://tinyurl.com/ydhotlaf
[4]: http://tinyurl.com/y6vkeo5t
[5]: http://tinyurl.com/ybn5oq9v
[6]: http://tinyurl.com/jubgged

## Discord

Feel free to join the [discord channel](https://discord.gg/4MymV4EaY5) for
discussion and chat with the developer.

*Copyright (c) 2019-2026 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*
