## Design and Architecture

The Q library comprises of two layers:

1. *q_lib*: The core DSP library, which has no dependencies except the
   standard c++ library. In the future, it is possible to make use additional
   libraries, as long as the libraries depended upon are also self-contained.
   *q_lib* is a no-frills, lightweight, header-only library.
2. *q_io*: Audio and MIDI I/O layer, with very minimal dependencies
   ([portaudio](http://www.portaudio.com/) and
   [portmidi](http://portmedia.sourceforge.net/portmidi/)) and very loose
   coupling via thin wrappers that are easy to transplant and port to a host,
   with or without an operating system, such as an audio plugin or direct to
   hardware ADC and DAC.

<img src="{{ site.url }}/q/assets/images/q-arch.png"
    width="50%"
    alt="q architecture" />

By design, communication to and from the application, including but not
limited to parameter control, is done exclusively via MIDI. We will track the
development of the forthcoming (as of January 2019) MIDI 2.0, especially
extended 16-bit and 32-bit resolution and MIDI Capability Inquiry (MIDI-CI)
"Universal System Exclusive" messages.

The architecture intuitively models real-world (hardware) effect processors
(and synthesizers) with *a)* zero or more input channels and one or more
output channels, and *b)* a means for communication and control via MIDI.
Such design simplicity is fundamental. There is very clear separation of
concerns. There are no graphical user interfaces. There are no direct
hardware or software controls. User interface is outside the scope of the
library. You deal with that elsewhere, or perhaps not at all.

Such design simplicity makes it easy for applications to be incorporated in
any hardware or software host. MIDI is a very simple protocol with a well
defined and evolving standard. The ability to use any MIDI controller (again
both hardware or software) to control an application is a very powerful and
intuitive concept. It is the very concept that gave MIDI widespread appeal
and ubiquity to begin with.

This simplified control scheme using MIDI also allows applications to be
easily testable in isolation —a very desirable capability not typically found
in more complex and monolithic systems.

---

*Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

