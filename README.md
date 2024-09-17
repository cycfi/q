# ![Q-Logo](docs/modules/ROOT/images/q-logo-small.png) Audio DSP Library

[![CMake Build Matrix](https://github.com/cycfi/q/workflows/Build/badge.svg)](https://github.com/cycfi/q/actions?query=workflow%3ABuild)

## Introduction

Q is a cross-platform C++ library for audio digital signal processing. Q is named after the "Q factor," a dimensionless parameter that describes the quality of a resonant circuit. The Q DSP Library is designed to be simple and elegant, as the simplicity of its name suggests, and efficient enough to run on small microcontrollers.

Q simplifies complex DSP programming tasks without sacrificing readability by leveraging the power of modern C++ and efficient use of functional programming techniques, especially function composition using fine-grained and reusable function objects (both stateless and stateful).

Q is the host of some experimental Music related DSP facilities [the author](#jdeguzman) has accumulated over the years as part of research and development, and will continue to evolve to accommodate more facilities necessary for the fulfillment of various Music related projects.

The library is Open Source and released under the very liberal [MIT license](http://tinyurl.com/p6pekvo).

> **NOTE:** The library has now reached version 1.0 and has stabilized. The documentation is in sync with the code in the master branch. Any future changes will be developed in feature branches and merged incrementally to the master branch. The develop branch will cease to exist. The API is now stable, and any changes will be documented accordingly.
>
> Versions will be in separate branches. The master branch will target the latest version. Currently, it targets v1.5, which will be a significant departure from v1.0, when completed. This update includes the retirement of the BACF (bitstream autocorrelation) based pitch detector and the introduction of a new, much better pitch detection algorithm with integrated onset detection.

**Update:** We're closing in towards v1.5. The Hz pitch detection system is presented in this article series: [Pitch Perfect: Enhanced Pitch Detection Techniques (Part 1)
](https://www.cycfi.com/2024/09/pitch-perfect-enhanced-pitch-detection-techniques-part-1/)

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

* q_lib has no dependencies except the standard c++ library.

The *q_io* layer provides cross-platform audio and MIDI host connectivity straight out of the box. The *q_io* layer is optional. The *q_lib* layer is usable without it. *q_io* is used in the tests and examples, but can be easily replaced by other mechanisms in an application.

## Documentation

* [Setup and Installation](https://cycfi.github.io/q/q/v1.5-dev/setup.html)
* [Quick Start](https://cycfi.github.io/q/q/v1.5-dev/quick_start.html)
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

*Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*



