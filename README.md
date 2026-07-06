# ![Q-Logo](docs/modules/ROOT/images/q-logo-small.png) Audio DSP Library

[![CMake Build Matrix](https://github.com/cycfi/q/workflows/Build/badge.svg)](https://github.com/cycfi/q/actions?query=workflow%3ABuild) [![Getting Started](https://github.com/cycfi/q/actions/workflows/getting-started.yml/badge.svg)](https://github.com/cycfi/q/actions/workflows/getting-started.yml)

## Introduction

Q is a cross-platform C++ library for audio digital signal processing. Q is named after the "Q factor," a dimensionless parameter that describes the quality of a resonant circuit. The Q DSP Library is designed to be simple and elegant, as the simplicity of its name suggests, and efficient enough to run on small microcontrollers.

Q simplifies complex DSP programming tasks without sacrificing readability by leveraging the power of modern C++ and efficient use of functional programming techniques, especially function composition using fine-grained and reusable function objects (both stateless and stateful).

Q is the host of some experimental Music related DSP facilities [the author](#jdeguzman) has accumulated over the years as part of research and development, and will continue to evolve to accommodate more facilities necessary for the fulfillment of various Music related projects.

The library is Open Source and released under the very liberal [Boost Software License, Version 1.0](https://www.boost.org/LICENSE_1_0.txt).

> **Status:** Q is developed on the `master` branch, which currently targets **v1.5**, a substantial evolution of v1.0. Feature branches merge in incrementally and the docs stay in sync with `master`; the API is stable, and changes are documented as they land.
>
> **New in v1.5:**
> - **Resonant filters:** a TPT state-variable filter, a Moog ladder, and a Chamberlin SVF
> - **Granular / overlap-add primitives:** the `grain` tap, `best_lag` self-similarity search, and interpolated fractional ring buffers (behind sustain, freeze, and PSOLA effects)
> - **Band-limited oscillators:** anti-aliased saw, square, pulse, and triangle
> - **True-RMS envelope follower** plus an expanded saturation/clipping family
> - A step-by-step **[Tutorials](https://cycfi.github.io/q/q/v1.5-dev/tutorials/index.html)** track
>
> These join the existing DSP toolkit: oscillators, envelopes, filters, dynamics, delays, pitch detection, FFT, and more.

## Overview

The Q library comprises of two layers:

<p align="center">
<img src="https://cycfi.github.io/q/q/v1.5-dev/_images/q-layers.svg" width="50%">
</p>

1. q_io: Audio and MIDI I/O layer. The q_io layer provides cross-platform audio and MIDI host connectivity straight out of the box. The q_io layer is optional. The q_lib layer is usable without it.

2. q_lib: The core DSP library, q_lib is a no-frills, lightweight, header-only library.

### Dependencies
The dependencies are determined by the arrows.

* q_io has very minimal dependencies ([portaudio](http://www.portaudio.com/) and
   [portmidi](http://portmedia.sourceforge.net/portmidi/)) with very loose coupling via thin wrappers that are easy to transplant and port to a host, with or without an operating system, such as an audio plugin or direct to hardware ADC and DAC.

* q_io is used in the tests and examples, but can be easily replaced by other mechanisms in an application. DAW (digital audio workstations), for example, have their own audio and MIDI I/O mechanisms.

* q_lib has no third-party dependencies. It uses only the C++ standard library and the header-only [Cycfi infra](https://github.com/cycfi/infra) support library.

The *q_io* layer provides cross-platform audio and MIDI host connectivity straight out of the box. The *q_io* layer is optional. The *q_lib* layer is usable without it. *q_io* is used in the tests and examples, but can be easily replaced by other mechanisms in an application.

You do not install these dependencies by hand. `infra` is Cycfi-owned and ships as a git submodule (clone with `--recurse-submodules`); PortAudio and PortMidi are downloaded automatically by CMake at configure time. See [Setup and Installation](https://cycfi.github.io/q/q/v1.5-dev/setup.html) for the full guide.

## Building

You need a C++20 compiler and [CMake](https://cmake.org/) 3.16 or higher. On Linux, also install the ALSA headers (`sudo apt-get install libasound2-dev`).

```sh
git clone --recurse-submodules https://github.com/cycfi/Q.git
cd Q
cmake -B build
cmake --build build
```

The first configure downloads PortAudio and PortMidi (and, if the submodule is absent, `infra`), so it takes a little longer than later runs. To check your setup, run `build/example/sin_osc/example_sin_osc`; it plays a five-second 440 Hz sine wave on the default audio output. Run the tests with `ctest --test-dir build`.

## Documentation

* [Setup and Installation](https://cycfi.github.io/q/q/v1.5-dev/setup.html)
* [Tutorials](https://cycfi.github.io/q/q/v1.5-dev/tutorials/index.html)
* [Fundamentals](https://cycfi.github.io/q/q/v1.5-dev/fundamentals.html)
* [Reference](https://cycfi.github.io/q/q/v1.5-dev/index.html)

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
[2]: https://ciere.com/
[3]: http://tinyurl.com/ydhotlaf
[4]: http://tinyurl.com/y6vkeo5t
[5]: http://tinyurl.com/ybn5oq9v
[6]: http://tinyurl.com/jubgged

## Discord

Feel free to join the [discord channel](https://discord.gg/4MymV4EaY5) for
discussion and chat with the developer.

*Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.*
*Distributed under the [Boost Software License, Version 1.0](https://www.boost.org/LICENSE_1_0.txt)*


