# ![Q-Logo](docs/modules/ROOT/images/q-logo-small.png) Audio DSP Library

[![CMake Build Matrix](https://github.com/cycfi/q/workflows/Build/badge.svg?branch=develop)](https://github.com/cycfi/q/actions?query=workflow%3ABuild)

## Introduction

Q is a cross-platform C++ library for audio digital signal processing. Q is named after the "Q factor," a dimensionless parameter that describes the quality of a resonant circuit. The Q DSP Library is designed to be simple and elegant, as the simplicity of its name suggests, and efficient enough to run on small microcontrollers.

Q simplifies complex DSP programming tasks without sacrificing readability by leveraging the power of modern C++ and efficient use of functional programming techniques, especially function composition using fine-grained and reusable function objects (both stateless and stateful).

Q is the host of some experimental Music related DSP facilities [the author](#jdeguzman) has accumulated over the years as part of research and development, and will continue to evolve to accommodate more facilities necessary for the fulfillment of various Music related projects.

The library is Open Source and released under the very liberal [MIT License](https://opensource.org/licenses/MIT).

> **Status:** `master` tracks the latest stable release (currently v1.0.2). The next release, **v1.5**, is developed on `develop`, where the docs stay in sync as changes land. See [NEWS.md](NEWS.md) for what is new in each release.
>
> **v1.5 highlights:** MIDI 1.0 and MIDI 2.0 complete (MPE, translation, endpoint discovery, MIDI-CI; working towards full MIDI 2.0 compliance), q_io moved to RtAudio and libremidi, FM synthesis with a DX7 patch compiler, virtual analog building blocks, QPlug for building CLAP, VST3 and AudioUnit plugins, and a return to the MIT License.
>
> Full release notes: [NEWS.md](NEWS.md).

## Highlights

* [Type-Safe Units: 440_Hz, 10_ms, -6_dB](https://cycfi.github.io/q/q/v1.5-dev/reference/units.html)
* [C++20 Concepts](https://cycfi.github.io/q/q/v1.5-dev/reference/support/basic_concepts.html)
* [Composable Function Objects](https://cycfi.github.io/q/q/v1.5-dev/fundamentals.html)
* [Header-Only, No Dependencies, Desktop to Microcontroller](https://cycfi.github.io/q/q/v1.5-dev/reference/q_lib.html)
* [Full MIDI 2.0: UMP, MPE, MIDI-CI, Verified with the MIDI Association Workbench](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/conformance.html)
* [BACF Pitch Detection: Sub-Cent Accuracy, Low Latency](https://cycfi.github.io/q/q/v1.5-dev/reference/pitch.html)
* [FM Synthesis That Plays Original DX7 Patches](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm.html)
* [Antialiased Virtual Analog Oscillators and Ladder Filters](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va.html)
* [One Implementation, Every Plugin Format: CLAP, VST3, AudioUnit, Standalone](https://cycfi.github.io/q/q/v1.5-dev/q_plug/index.html)
* [Cross-Platform Audio and MIDI I/O](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io.html)

## Overview

The Q library comprises of three layers:

<p align="center">
<img src="docs/modules/ROOT/images/q-layers.svg" width="680">
</p>

1. q_plug: QPlug, the audio plugin layer. QPlug builds a Q processor into CLAP, VST3 and AudioUnit plugins, with an [Elements](https://github.com/cycfi/elements) GUI. It is optional and off by default.

2. q_io: Audio and MIDI I/O layer. The q_io layer provides cross-platform audio and MIDI host connectivity straight out of the box. The q_io layer is optional. The q_lib layer is usable without it.

3. q_lib: The core DSP library, q_lib is a no-frills, lightweight, header-only library.

### Dependencies
Each layer sits on the one below it: QPlug and QIO each build on Q, and neither needs the other. The boxes inside a layer are what it depends on.

* q_plug depends on [Elements](https://github.com/cycfi/elements), [CLAP](https://github.com/free-audio/clap), [clap-wrapper](https://github.com/free-audio/clap-wrapper) and [nlohmann json](https://github.com/nlohmann/json). CMake fetches them only when QPlug is built, with `-DQ_BUILD_PLUG=ON`.

* q_io has very minimal dependencies ([RtAudio](https://github.com/thestk/rtaudio) and
   [libremidi](https://github.com/celtera/libremidi)) with very loose coupling via thin wrappers that are easy to transplant and port to a host, with or without an operating system, such as an audio plugin or direct to hardware ADC and DAC.

* q_io is used in the tests and examples, but can be easily replaced by other mechanisms in an application. DAW (digital audio workstations), for example, have their own audio and MIDI I/O mechanisms.

* q_lib has no third-party dependencies. It uses only the C++ standard library and the header-only [Cycfi infra](https://github.com/cycfi/infra) support library.

You do not install these dependencies by hand. `infra` is Cycfi-owned and ships as a git submodule (clone with `--recurse-submodules`); RtAudio and libremidi are downloaded automatically by CMake at configure time, and so are QPlug's dependencies when it is built. See [Setup and Installation](https://cycfi.github.io/q/q/v1.5-dev/setup.html) for the full guide.

## Building

You need a C++20 compiler and [CMake](https://cmake.org/) 3.16 or higher. On Linux, also install the ALSA headers (`sudo apt-get install libasound2-dev`).

```sh
git clone --recurse-submodules https://github.com/cycfi/Q.git
cd Q
cmake -B build
cmake --build build
```

The first configure downloads RtAudio and libremidi (and, if the submodule is absent, `infra`), so it takes a little longer than later runs. Add `-DQ_BUILD_PLUG=ON` to build QPlug and its example plugins as well. To check your setup, run `build/example/sin_osc/example_sin_osc`; it plays a five-second 440 Hz sine wave on the default audio output. Run the tests with `ctest --test-dir build`.

## Documentation

* [Setup and Installation](https://cycfi.github.io/q/q/v1.5-dev/setup.html)
* [Q Tutorials](https://cycfi.github.io/q/q/v1.5-dev/tutorials/index.html)
* [QPlug Tutorials](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/index.html)
* [Fundamentals](https://cycfi.github.io/q/q/v1.5-dev/fundamentals.html)
* [Reference](https://cycfi.github.io/q/q/v1.5-dev/index.html)
* [QPlug Reference](https://cycfi.github.io/q/q/v1.5-dev/q_plug/index.html)

## <a name="jdeguzman"></a>About the Author

<img align="right" src="https://github.com/cycfi/elements/blob/assets/images/joel.jpg?raw=true" width="200">

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
[2]: https://ciere.com/
[3]: http://tinyurl.com/ydhotlaf
[4]: http://tinyurl.com/y6vkeo5t
[5]: http://tinyurl.com/ybn5oq9v
[6]: http://tinyurl.com/jubgged

## Discord

Feel free to join the [discord channel](https://discord.gg/4MymV4EaY5) for
discussion and chat with the developer.

*Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*


