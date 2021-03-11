# ![Q-Logo](docs/assets/images/q-logo-small.png) Audio DSP Library

[![CMake Build Matrix](https://github.com/cycfi/q/workflows/Build/badge.svg)](https://github.com/cycfi/q/actions?query=workflow%3ABuild)

## Introduction

Q is a cross-platform C++ library for Audio Digital Signal Processing. Aptly
named after the "Q factor", a dimensionless parameter that describes the
quality of a resonant circuit, the Q DSP Library is designed to be simple and
elegant, as the simplicity of its name suggests, and efficient enough to run
on small microcontrollers.

Q leverages the power of modern C++ and efficient use of functional
programming techniques, especially function composition using fine-grained
and reusable function objects (both stateless and stateful), to simplify
complex DSP programming tasks without sacrificing readability.

Q is the host of some experimental Music related DSP facilities such as
[Virtual Pickups](http://tinyurl.com/y8cqt8jr) (Virtual pickup placement
simulator) and [Bitstream Autocorrelation](http://tinyurl.com/yb49zlld) (An
extremely fast and efficient pitch detection scheme) [the author](#jdeguzman)
has accumulated over the years as part of research and development, and will
continue to evolve to accommodate more facilities necessary for the
fulfillment of various Music related projects.

The library is Open Source and released under the very liberal [MIT
license](http://tinyurl.com/p6pekvo).

## Announcement

An apology. I am an advocate of Open Source. But after having shared open-source 
projects for over 20 years now, starting with my contributions to the [Boost libraries][8], 
it pains me to say that I am closing some source code for pragmatic concerns. The source 
code of technologies that are in very active research and development will be closed — 
that would be pitch detection and onset detection. If you are using the pitch detection 
source code now, I suggest forking the latest version with the current MIT license. 
I will close the pitch detection source code by mid-April.

Please follow this link for more info: [Rethinking Open Source][9]. 

## News

- 11 March 2021: BACF pitch detection is going away. See announcement above.
- 4 February 2021: Posted [Fast and Efficient Pitch Detection: Power of Two][7].
  Bitstream Autocorrelation (BACF) is fast and accurate. What can be better? Well, 
  two BACFs in parallel!
- 6 July 2020: Posted [Fast and Efficient Pitch Detection: Revisited][1]. 
  A long overdue article about the technical details of BACF.
- 25 June 2020: Pitch/period detection improvements now in master branch.
- 19 June 2020: Pitch/period detection improvements in develop branch (for
  now until it becomes stable). Mostly bug fixes; esp. the ability to handle
  higher frequencies and bigger low-high ranges.
- 18 May 2020: We're getting closer to a 1.0 release. Busy working on the
  docs, including small improvements on the example code.
  
[1]: https://www.cycfi.com/2020/07/fast-and-efficient-pitch-detection-revisited

## Discord

Feel free to join the [discord channel](https://discord.gg/4MymV4EaY5) for 
discussion and chat with the developer.

## Documentation

Documentation is work in progress. Stay tuned...

1. [Design and Architecture](https://cycfi.github.io/q/design)
2. [Setup and Installation](https://cycfi.github.io/q/setup)
3. [Hello, World](https://cycfi.github.io/q/hello_world)
4. [Hello, Universe](https://cycfi.github.io/q/hello_universe)
5. [Fundamentals](https://cycfi.github.io/q/fundamentals)

## <a name="jdeguzman"></a>About the Author

Joel got into electronics and programming in the 80s because almost
everything in music, his first love, is becoming electronic and digital.
Since then, he builds his own guitars, effect boxes and synths. He enjoys
playing distortion-laden rock guitar, composes and produces his own music in
his home studio.

Joel de Guzman is the principal architect and engineer at [Cycfi Research][1]
and a consultant at [Ciere Consulting][2]. He is a software engineer
specializing in advanced C++ and an advocate of Open Source. He has authored
a number of highly successful Open Source projects such as [Boost.Spirit][3],
[Boost.Phoenix][4] and [Boost.Fusion][5]. These libraries are all part of the
[Boost Libraries][6], a well respected, peer-reviewed, Open Source,
collaborative development effort.

[1]: https://www.cycfi.com/
[2]: https://ciere.com/
[3]: http://tinyurl.com/ydhotlaf
[4]: http://tinyurl.com/y6vkeo5t
[5]: http://tinyurl.com/ybn5oq9v
[6]: http://tinyurl.com/jubgged
[7]: https://bit.ly/3cFkR8E
[8]: https://www.boost.org/
[9]: https://www.cycfi.com/2021/03/rethinking-open-source/

---

*Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

