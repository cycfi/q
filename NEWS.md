# Release notes

High-level, user-facing changes only. For the day-to-day detail, see
`docs/dev_log.md` (internal, not part of this file).

## v1.5.0 (2026-10-05)

**MIDI, complete.** MIDI 1.0 is finished (sysex, RPN/NRPN, 14-bit
controllers, channel mode), and MIDI 2.0 is added in full: Universal MIDI
Packets, per-note expression, MPE, translation between the two protocols in
both directions, endpoint and function block discovery, and MIDI-CI with
profiles and property exchange, both answering and asking. Tested as part
of a device under the MIDI Association's MIDI 2.0 Workbench on macOS and
Linux, over a virtual port and as a USB MIDI 2.0 device, and on Windows 11
as the USB device, with no errors;
see the [MIDI 2.0 Conformance](https://cycfi.github.io/q/q/v1.5/reference/midi/conformance.html)
page for what was covered.

**q_io moved to RtAudio and libremidi**, replacing PortAudio and PortMidi.
The public API is unchanged; a new `audio_device::default_id` follows
whatever device the OS has set as its default. q_io also gained a MIDI
output stream, MIDI 2.0 packet streams, and standard MIDI file reading.

**New synthesis:** FM synthesis with a DX7 patch compiler that plays the
original factory patches, and virtual analog building blocks (a
configurable ladder filter, an analog-style oscillator core, hard sync).
Both ship with an example and a tutorial.

**Runs on microcontrollers.** The core needs only a C++20 compiler and
the standard library and the header-only Cycfi infra; it is tested on STM32 with Arm GCC 12.

**Reference documentation is now complete.** Every public component has a
reference page with a figure checked against the library's own output.

**QPlug joins Q** as `q_plug`, a third layer beside q_io: write the DSP,
the parameters and an Elements GUI as three plain classes, and get CLAP,
VST3 and AudioUnit plugins and a standalone app. It is off by default;
build it with `-DQ_BUILD_PLUG=ON`. Seven tutorials go from a one parameter
gain to Dexter, a DX7 style synth that plays the original patches. Plugins
build on any drawing backend Elements offers; on Windows, Skia and Cairo
link statically, so a plugin loads in any host. An editor that cannot start,
for want of a usable OpenGL say, is refused rather than taking the host
down.

**Two deprecations:** `leaky_integrator` (use `one_pole_lowpass`, which is
the same filter with an exact pole), and the old `clip`/`soft_clip` names
(use `hard_clip`/`cubic_clip`).

**Tested everywhere it runs.** CI builds and tests every platform and
drawing backend, and weekly under the address, undefined behavior and
thread sanitizers.

Also: resonant filters (state variable, Moog and OTA ladders), granular building blocks
(`grain`, `best_lag`, interpolated fractional ring buffers), band-limited
oscillators, a true-RMS envelope follower, and a step-by-step
[Tutorials](https://cycfi.github.io/q/q/v1.5/tutorials/index.html)
track.

## v1.0.2 (2026-09-29)

CI fixes only: Windows builds pinned to Visual Studio 2022, and the
documentation site published on request. The library is unchanged from
v1.0.1.

## v1.0.1 (2025-08-03)

Build fixes only: a Clang/libc++ compiler-flag concatenation bug, and q_io
failing to find `portaudio.h` on some setups.

## v1.0.0 (2024-08-21)

First numbered release.
