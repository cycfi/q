# Release notes

High-level, user-facing changes only. For the day-to-day detail, see
`docs/dev_log.md` (internal, not part of this file).

## v1.5.0 (in development, on `develop`)

**MIDI, complete.** MIDI 1.0 is finished (sysex, RPN/NRPN, 14-bit
controllers, channel mode), and MIDI 2.0 is added in full: Universal MIDI
Packets, per-note expression, MPE, translation between the two protocols in
both directions, endpoint discovery, and MIDI-CI. Q is working towards full
MIDI 2.0 compliance.

**q_io moved to RtAudio and libremidi**, replacing PortAudio and PortMidi.
The public API is unchanged; a new `audio_device::default_id` follows
whatever device the OS has set as its default. q_io also gained a MIDI
output stream, MIDI 2.0 packet streams, and standard MIDI file reading.

**New synthesis:** FM synthesis with a DX7 patch compiler that plays the
original factory patches, and virtual analog building blocks (a
configurable ladder filter, an analog-style oscillator core, hard sync).
Both ship with an example and a tutorial.

**Reference documentation is now complete.** Every public component has a
reference page with a figure checked against the library's own output.

**QPlug joins Q** as `q_plug`, a third layer beside q_io: write the DSP,
the parameters and an Elements GUI as three plain classes, and get CLAP,
VST3 and AudioUnit plugins and a standalone app. It is off by default;
build it with `-DQ_BUILD_PLUG=ON`.

**Back to the MIT License.** Q returns from BSL-1.0 to MIT, the license
it carried through v1.0.

**Two deprecations:** `leaky_integrator` (use `one_pole_lowpass`, which is
the same filter with an exact pole), and the old `clip`/`soft_clip` names
(use `hard_clip`/`cubic_clip`).

Also: state-variable resonant filters (SVF, Moog ladder), the PSOLA family
(`grain`, `best_lag`, interpolated fractional ring buffers), band-limited
oscillators, a true-RMS envelope follower, and a step-by-step
[Tutorials](https://cycfi.github.io/q/q/v1.5-dev/tutorials/index.html)
track.

## v1.0.1 (2025-08-03)

Build fixes only: a Clang/libc++ compiler-flag concatenation bug, and q_io
failing to find `portaudio.h` on some setups.

## v1.0.0 (2024-08-21)

First numbered release.
