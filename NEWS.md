# Release notes

High-level, user-facing changes only. For the day-to-day detail, see
`docs/dev_log.md` (internal, not part of this file).

## v1.6.0 (beta 1, on `develop`)

APIs marked *(unstable)* are new in this release and may change in any
release until they mature.

### Breaking changes

Code written for v1.5 may need these changes.

- **The FFT is an object.** The compile-time functions `q::fft<N>`,
  `ifft<N>`, `rfft<N>`, `irfft<N>` and `magspec<N>` are gone. In their
  place, `q::fft` (complex) and `q::rfft` (real) are built once with their
  size and transform in place: `q::fft f{N};` then `f.forward(data)` and
  `f.inverse(data)`. The data layouts and the scaling are unchanged.
  `basic_fft<double>` and `basic_rfft<double>` cover double. Build the
  object outside the audio callback, since it allocates; a transform never
  does. See the FFT reference page, "Migrating from v1.5".
- **`decibel` is a float unit** (it was double), and `dB()`, `_dB` and
  `lin_to_db` give float.
- **`lin_to_db` and `lin_float` read new tables**, exact to about 1e-5 dB
  over the whole float range (the old ones clamped at 120 dB and were
  coarser), so their results differ slightly.
- **The RMS envelope followers' floor is -120 dB** (it was -60 dB): levels
  below -60 dB now read true instead of as silence.
- **`ar_envelope_follower::attack` and `release` take a `duration`**, such
  as `10_ms`, instead of float seconds.
- **`expander`'s ratio follows the 1:n notation**: below the threshold the
  output falls n dB per dB, so a ratio of 1 does nothing. The same ratio
  expands less than it did.
- **`hilbert_quadrature`'s q output changed sign**: q lags i by 90 degrees,
  so `atan2(q, i)` increases with time.
- **The ladder cells are public**: `q::detail::moog_cell` and
  `q::detail::ota_cell` are `q::moog_cell` and `q::ota_cell`.
- **Dividing a number by a unit no longer compiles.** `1 / frequency`
  returned a frequency, but the reciprocal of a unit is not that unit.
- **Templates on the sample type.** `sin_cos_gen`, the Hann, Hamming,
  Blackman and exponential generators, the RMS envelope followers and
  `dx_envelope_gen` are now `basic_` templates on T, their old names the
  double form, so a single-precision FPU can run them in float.
  `basic_fm_operator` takes a third parameter, the ratio's type. Code that
  forward-declares one of these classes no longer compiles.
- **`sin_cos_gen` runs in double**: it returns `std::pair<double, double>`
  (it was float). `poly_blamp` returns float (it was double).
- **`pitch_detector` is `bacf_pitch_detector`**, in
  `<q/pitch/bacf_pitch_detector.hpp>`. The old name stays as a deprecated
  alias.
- **A full `zero_crossing_collector` drops its oldest edge** instead of
  resetting. `zero_crossing_ex::pulse_width_diff` is gone.
- **q_io's `audio_stream`** returns its `time()` and latencies as
  `cycfi::duration` whatever the include order.
- **q_io's MIDI device ids are stable**: a port keeps its id across
  listings and gets it back when it returns. An unknown audio device id
  gives the default device, and departed audio devices are not listed.
- **Subnormals flush to zero** inside q_io's audio callback and QPlug's
  `process` and parameter flush, with the new `flush_denormals` guard.
- **QPlug**: a direct subclass of `base_plugin` must implement
  `view_aspect_ratio()`. The zoom range is 0.75 to 1.5 (it was 0.5 to 2).
- **CMake 3.22** is the minimum throughout (it was 3.16).

### New

**Pitch shifting** *(unstable)*. `bacf_pitch_shifter` raises or lowers
the pitch of a sound as it plays, by a ratio that may change every
sample, on single notes and chords alike. It is a SOLA shifter whose
splices are placed by bitstream autocorrelation, and at a ratio of 1 it
passes the input with no latency. `varispeed_tap` is the delay line it
reads, played at a varying speed. A new `whammy` example drives it from
MIDI pitch bend. See the Pitch Shifting reference pages.

**Formant preservation** *(unstable)*. `formant_learner` learns an
instrument's fixed spectral envelope from its playing and fits a
`formant_table`; `formant_corrector`, after the shifter, restores the
instrument's own formants at the current ratio, with no latency.

**Spectral tools** *(unstable)*. `spectral_learner` learns a sound's
long-term spectrum and its spectral envelope. `lpc_fit` models a spectral
envelope as an all-pole filter. `eq_matcher` gives a sound the spectral
envelope of another, sample by sample, with no latency. `convolver`
applies an impulse response, partitioned, with no latency.

**Pitch tracking and attacks** *(unstable)*. `bacf_lag_tracker` follows
the lag at which a signal repeats, for a time-domain splicer.
`transient_detector` fires at each attack, within a fraction of a
millisecond on a plucked string.

**Faster FFTs.** Float transforms run on PFFFT, a SIMD FFT, about 4x
faster than Q's own. It is header-only as Q uses it and on by default;
`-DQ_USE_PFFFT=OFF` leaves it out. Q's own FFT, used for double and on
microcontrollers, is also new: about 3x faster than v1.5's on an M2 and
twice as fast again on a Cortex-M7. `rfft` and `irfft` transform real
signals directly.

**Real-time utilities** *(unstable)*. `handoff` sends objects that need
allocation, such as a sample buffer, from one thread to another without
a lock. `ring_allocator` hands out blocks of any size from one buffer to
one thread and takes them back from another. `tasks` runs work posted
from one thread on another, with no lock, wait or allocation at either
end. `audio_clock` counts the frames the audio thread has processed. In
QPlug, `controller::post` runs work on the main thread, now or after a
delay. Two new QPlug tutorials, the Sampler and the Looper, use them.

**Also new:** `one_pole_highpass` *(unstable)*; `flush_denormals`
*(unstable)*, a scoped guard that flushes subnormals to zero on its
thread; a mirror length for `mirrored_ring_buffer`, so a window of a few
samples needs only that many extra elements; copy assignment for
`biquad`, `peak` and `schmitt_trigger`.

**QPlug.** An editor whose content fits its view keeps its proportions as
the window is resized, and reopens at the size it was left. A plugin's
view resizes when the host grants a resize without asking for a size.
Preset writes are atomic: a failed write leaves the presets as they were.
macOS bundles are signed ad hoc, which Ableton Live requires. A plugin
example's library is named after the plugin, and IDEs show Q's targets
and Elements' files in groups of their own.

### Fixed

- `pitch_names`' E flat held E's frequencies.
- Mixing two units picked the first one's value type, not the larger.
- `period.hpp` and `frequency.hpp` failed to compile in one include order.
- `bitset` accepted an index one past its size, and set bits with a 32-bit
  mask on 64-bit words; `bitstream_acf` mis-sized one-word bitsets.
- `moving_sum::fill` did not reset its position, and multiplied in
  unsigned arithmetic; `moving_maximum` started above the lowest float.
- `onset_gate`'s slope went stale while the gate was open;
  `signal_conditioner` left a member uninitialized;
  `fast_rms_envelope_follower_db` took the log of 0.
- `zero_crossing_ex` measured its peak from the offset signal.
- `adsr_envelope_gen::sustain_level` overwrote a sustain's end level;
  `lfo_gen::key_on` with a delay and no fade started at full;
  `blackman_gen::midpoint` did not reset its state.
- MIDI 1.0: a sysex manufacturer id's high byte was not masked to 7 bits.
  MIDI 2.0: an overflowing flex-data message left a stale partial one.
- `hilbert_quadrature.hpp` did not include what it needs.
- q_io reports no MIDI 2.0 where libremidi was built without it, instead
  of opening ports nothing sees.
- Float literals replace stray doubles in `saw_osc`, `poly_blamp`,
  `zero_crossing_collector`, `level_crossfade` and
  `soft_knee_compressor`, so a single-precision FPU runs them in hardware.

### Build and platforms

- Windows on Arm64 builds.
- Q builds as position-independent code when QPlug is on.
- libremidi is back on upstream, with its JACK backend left out.
- clap-wrapper moves to 0.17.0, with fixes for multi-config generators
  and for macOS Ninja builds.
- Elements moves to a newer develop commit (`fit`, a Direct2D resize
  fix). Dexter follows the `swatch_highlighter` and `region_highlighter`
  rename.

## v1.5.4 (2026-10-09)

**QPlug: building a plugin outside Q on Linux.** v1.5.3 could not link a
plugin built in a directory of its own, the way the Gain tutorial starts
one: the linker was handed a path to its export map that did not exist.

## v1.5.3 (2026-10-09)

**QPlug: two plugins in one host, on Linux and macOS.** On Linux, a host
with two QPlug plugins crashed on exit, since the plugins shared the
copies of quill, json and Elements each carries; a plugin now exports its
entry points and nothing else. Closing an editor could also end the host
there, with an X error, when the editor's window outlived the host's. On
macOS, every plugin defined the same window class. Found by testing two
plugins with their editors open in one process, on every platform.

## v1.5.2 (2026-10-09)

**QPlug on Windows: two plugins in one host.** With the editors of two
different QPlug plugins open in one host, the second editor reported
"Could not register class" and ran on the first plugin's window code,
and the host crashed on exit. Each plugin now registers its window
classes under its own module, and Artist's Direct2D factories live as
long as the process. Found running Dexter and Gain in REAPER on Windows 10.

## v1.5.1 (2026-10-07)

**Docs only.** The minimum hardware for microcontrollers is stated: a
hardware FPU, on ARM a Cortex-M4F or better. Setup and Installation is
split into Setup, Microcontrollers and Testing pages, and its landing page
lists every page under it.

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
