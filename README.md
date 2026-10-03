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

## Features

* **Units:** [Frequency](https://cycfi.github.io/q/q/v1.5-dev/reference/units/frequency.html) · [Duration](https://cycfi.github.io/q/q/v1.5-dev/reference/units/duration.html) · [Period](https://cycfi.github.io/q/q/v1.5-dev/reference/units/period.html) · [Phase](https://cycfi.github.io/q/q/v1.5-dev/reference/units/phase.html) · [Phase Iterator](https://cycfi.github.io/q/q/v1.5-dev/reference/units/phase_iterator.html) · [Decibel](https://cycfi.github.io/q/q/v1.5-dev/reference/units/decibel.html) · [Interval](https://cycfi.github.io/q/q/v1.5-dev/reference/units/interval.html) · [Pitch](https://cycfi.github.io/q/q/v1.5-dev/reference/units/pitch.html) · [Pitch Names](https://cycfi.github.io/q/q/v1.5-dev/reference/units/pitch_names.html) · [Literals](https://cycfi.github.io/q/q/v1.5-dev/reference/units/literals.html)
* **Biquad Filters:** [Low Pass](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/lowpass.html) · [High Pass](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/highpass.html) · [Band Pass (constant skirt)](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/bandpass_csg.html) · [Band Pass (constant peak)](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/bandpass_cpg.html) · [All Pass](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/allpass.html) · [Notch](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/notch.html) · [Peaking](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/peaking.html) · [Low Shelf](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/lowshelf.html) · [High Shelf](https://cycfi.github.io/q/q/v1.5-dev/reference/biquad/highshelf.html)
* **Resonant and Other Filters:** [State Variable](https://cycfi.github.io/q/q/v1.5-dev/reference/resonant/svf.html) · [Moog Ladder](https://cycfi.github.io/q/q/v1.5-dev/reference/resonant/moog_ladder.html) · [OTA Ladder](https://cycfi.github.io/q/q/v1.5-dev/reference/resonant/ota_ladder.html) · [Chamberlin](https://cycfi.github.io/q/q/v1.5-dev/reference/resonant/chamberlin_filter.html) · [One Pole Low Pass](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/one_pole_lowpass.html) · [DC Block](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/dc_block.html) · [All-Pass Phase Shifters](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/allpass.html) · [Hilbert Quadrature](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/hilbert_quadrature.html)
* **Envelope Followers:** [Peak](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/peak_envelope_follower.html) · [Attack-Release](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/ar_envelope_follower.html) · [Fast](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/fast_envelope_follower.html) · [Fast Averaging](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/fast_ave_envelope_follower.html) · [Fast RMS](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/fast_rms_envelope_follower.html) · [True RMS](https://cycfi.github.io/q/q/v1.5-dev/reference/envelope/true_rms_envelope_follower.html)
* **Dynamics:** [Compressor](https://cycfi.github.io/q/q/v1.5-dev/reference/dynamic/compressor.html) · [Soft Knee Compressor](https://cycfi.github.io/q/q/v1.5-dev/reference/dynamic/soft_knee_compressor.html) · [Expander](https://cycfi.github.io/q/q/v1.5-dev/reference/dynamic/expander.html) · [AGC](https://cycfi.github.io/q/q/v1.5-dev/reference/dynamic/agc.html) · [Noise Gate](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/noise_gate.html) · [Clippers](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/clip.html)
* **Oscillators:** [Sine](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/sin_osc.html) · [Saw](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/saw_osc.html) · [Square](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/square_osc.html) · [Pulse](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/pulse_osc.html) · [Triangle](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/triangle_osc.html) · [Analog Oscillator](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/analog_osc.html) · [Hard Sync](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/va/sync.html) · [Antialiasing](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/antialiasing.html)
* **FM Synthesis:** [FM Operator](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/fm_operator.html) · [FM Routing](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/fm_routing.html) · [FM Algorithm](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/fm_algorithm.html) · [FM Voice](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/fm_voice.html) · [DX Envelope Generator](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/dx_envelope_gen.html) · [DX7 Patch Compiler](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/dx_patcher.html) · [DX7 SysEx](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/fm/dx_sysex.html)
* **Generators:** [Sine Cosine](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/sin_cos_gen.html) · [Blackman Window](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/blackman_gen.html) · [Hann Window](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/hann_gen.html) · [Hamming Window](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/hamming_gen.html) · [Linear Ramp](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/linear_gen.html) · [Exponential Ramp](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/exponential_gen.html) · [Envelope](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/envelope_gen.html) · [Noise](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/noise_gen.html) · [LFO](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/gen/lfo_gen.html)
* **Granular and Pitch Shifting:** [Grain](https://cycfi.github.io/q/q/v1.5-dev/reference/synth/grain.html) · [Best Lag](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/best_lag.html) · [Fractional Ring Buffer](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/fractional_ring_buffer.html) · [Sample Interpolation](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/sample_interpolation.html)
* **Pitch Detection:** [BACF Pitch Detector](https://cycfi.github.io/q/q/v1.5-dev/reference/pitch/pitch_detector.html) · [BACF Period Detector](https://cycfi.github.io/q/q/v1.5-dev/reference/pitch/bacf_period_detector.html) · [Bitstream Autocorrelation](https://cycfi.github.io/q/q/v1.5-dev/reference/pitch/bitstream_acf.html) · [Zero Crossing Collector](https://cycfi.github.io/q/q/v1.5-dev/reference/pitch/zero_crossing_collector.html)
* **Onset and Analysis:** [Onset Gate](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/onset_gate.html) · [Delta Gate](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/delta_gate.html) · [Spectral Flux](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/spectral_flux.html) · [Peak](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/peak.html) · [Peak Picker](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/peak_picker.html) · [Zero Crossing](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/zero_crossing.html) · [Schmitt Trigger](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/schmitt_trigger.html) · [Window Comparator](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/window_comparator.html) · [Edge Detectors](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/edge.html) · [Monostable](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/monostable.html) · [Signal Conditioner](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/signal_conditioner.html) · [FFT](https://cycfi.github.io/q/q/v1.5-dev/reference/spectral/fft.html)
* **Signal Processing:** [Delay](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/delay.html) · [Sample Hold](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/sample_hold.html) · [Moving Sum](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/moving_sum.html) · [Moving Average](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/moving_average.html) · [Moving Maximum](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/moving_maximum.html) · [Dynamic Smoother](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/dynamic_smoother.html) · [Integrator](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/integrator.html) · [Differentiators](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/differentiator.html) · [Median](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/median3.html) · [Map](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/map.html) · [Fast Downsample](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/fast_downsample.html) · [Level Crossfade](https://cycfi.github.io/q/q/v1.5-dev/reference/misc/level_crossfade.html)
* **Support:** [Ring Buffer](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/ring_buffer.html) · [Sample Format Conversion](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/float_convert.html) · [Fast Math](https://cycfi.github.io/q/q/v1.5-dev/reference/support/fast_math.html) · [Multi Buffer](https://cycfi.github.io/q/q/v1.5-dev/reference/support/multi_buffer.html) · [Audio Stream](https://cycfi.github.io/q/q/v1.5-dev/reference/support/audio_stream.html) · [Bitset](https://cycfi.github.io/q/q/v1.5-dev/reference/utility/bitset.html)
* **MIDI 1.0:** [Messages](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/messages.html) · [Controller Numbers](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/controller_numbers.html) · [Notes](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/notes.html) · [MIDI Processor](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/processor.html) · [Readers](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/readers.html) · [Stages: RPN, NRPN, 14 Bit, Channel Modes](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/stages.html)
* **MIDI 2.0:** [Universal MIDI Packets](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/midi2_messages.html) · [Per-Note Expression and MPE](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/per_note.html) · [MIDI 1.0 and 2.0 Translation](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/translation.html) · [Endpoint and Function Block Discovery](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/endpoint.html) · [MIDI-CI: Discovery, Profiles, Property Exchange](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/ci.html) · [Conformance](https://cycfi.github.io/q/q/v1.5-dev/reference/midi/conformance.html)
* **QIO:** [Audio Device](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/audio_device.html) · [Audio Stream](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/audio_stream.html) · [Audio File](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/audio_file.html) · [MIDI Device](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/midi_device.html) · [MIDI Stream](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/midi_stream.html) · [MIDI 2.0 Stream](https://cycfi.github.io/q/q/v1.5-dev/reference/q_io/midi2_stream.html)
* **QPlug:** [CLAP, VST3 and AudioUnit](https://cycfi.github.io/q/q/v1.5-dev/q_plug/architecture.html) · [Standalone App](https://cycfi.github.io/q/q/v1.5-dev/q_plug/setup.html) · [Parameters](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/parameter.html) · [Controller](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/controller.html) · [Processor](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/processor.html) · [MIDI](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/midi.html) · [Elements GUI](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/presenter.html) · [Header and Presets](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/header.html) · [Zoom and Display Scale](https://cycfi.github.io/q/q/v1.5-dev/q_plug/reference/zoom.html)
* **Q Tutorials:** [List Devices](https://cycfi.github.io/q/q/v1.5-dev/tutorials/list_devices.html) · [Sine Oscillator](https://cycfi.github.io/q/q/v1.5-dev/tutorials/sin_osc.html) · [Waveforms](https://cycfi.github.io/q/q/v1.5-dev/tutorials/waveforms.html) · [Delay](https://cycfi.github.io/q/q/v1.5-dev/tutorials/delay.html) · [IO Delay](https://cycfi.github.io/q/q/v1.5-dev/tutorials/io_delay.html) · [MIDI Monitor](https://cycfi.github.io/q/q/v1.5-dev/tutorials/midi_monitor.html) · [Square Synth](https://cycfi.github.io/q/q/v1.5-dev/tutorials/square_synth.html) · [Polyphonic Synth](https://cycfi.github.io/q/q/v1.5-dev/tutorials/poly_synth.html) · [Virtual Analog Synth](https://cycfi.github.io/q/q/v1.5-dev/tutorials/va_synth.html) · [FM Synth](https://cycfi.github.io/q/q/v1.5-dev/tutorials/fm_synth.html) · [Grain Freeze](https://cycfi.github.io/q/q/v1.5-dev/tutorials/grain_freeze.html) · [Sustain Hold](https://cycfi.github.io/q/q/v1.5-dev/tutorials/sustain_hold.html)
* **QPlug Tutorials:** [Gain](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/gain.html) · [Anna I: The Voice](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/anna_1.html) · [Anna II: The Filter](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/anna_2.html) · [Anna III: The Chorus](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/anna_3.html) · [Anna IV: Drawn Envelopes](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/anna_4.html) · [Anna V: Presets](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/anna_5.html) · [Dexter: DX7 Synth](https://cycfi.github.io/q/q/v1.5-dev/q_plug/tutorials/dexter.html)

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


