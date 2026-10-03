# q dev log

Internal. Dated, newest first, with commit hashes on develop. Not published
(lives outside `modules/`, so the Antora build ignores it). Significant
updates only: what changed and why, in a few sentences. The mechanics are in
the code and the reference pages, and the longer story of an effort in its
KB note.

Trimmed on 2026-09-28, when every entry was also pointed at develop's
commits; the feature branches they first cited were squash-merged and no
longer resolve. The full text before the trim: `git show
308ddccb:docs/dev_log.md`.

## 2026-10-03

`ca29e0e0` `remote_device::supports(category)`, true when a device
declared a MIDI-CI category in its reply. M2-101 v1.2 does not forbid
asking outside the declared categories (5.5.2; the device answers with a
NAK, 5.11), so the initiators still send what they are given and the check
is the caller's; the endpoint example uses it. `46299fd7` documents it.

Release preparation for v1.5, docs only:

- `8df6236d`, then `42e36b06`: a Highlights list on the README and the
  docs landing page, ten linked titles on what sets Q apart (units,
  concepts, composition, header-only, MIDI 2.0, BACF, DX7 FM, VA, one
  plugin for every format, I/O). Its wording is still open.
- `85352a70`: the missing v1.0.2 entry in NEWS.
- QPlug pages, the doc plan's items 19 to 23, 25 and 26:
  - `bbdb5ea7`: images moved from `images/qplug/` to `images/q_plug/`.
  - `b6345010`: the QPlug landing page includes `common.adoc`, links Q in
    the docs and gains Where to Go Next; Setup includes `common.adoc`.
  - `ab5d8bc8`: title-case headings, backticked option and example names,
    `ctl` throughout the presenter's notation, Dexter in the examples list.
  - `9f946ab0`: personified wording removed (code no longer sees, knows,
    asks or wants), 56 sentences on 14 pages.
  - `9558d53e`: the early-development warning is now a note that the API
    may change between minor releases.
  - `61fda49f`: two reference pages, Plugin Entry Points (the four
    functions and every `plugin_info` field) and Data Streams (`ostream`
    and `istream`, with a hostless save and load).
  - `e2c7c73c`: The Standalone App in Setup (devices, the Audio/MIDI
    Settings window per platform, where settings and state are kept, read
    from clap-wrapper's source at Q's pin); every tutorial links it.
  - `b4a3c8c1`: Linux in Setup (packages from CI, products, tests need
    an X11 display) and in Testing.
  - `ad025c1d`: one home per subject. The presenter page points to Zoom
    and The Header instead of repeating their tables, Setup to
    Architecture's Dependencies and to Testing (the validator options
    moved there), and the landing page and Architecture to Entry Points.
- `350104f8`: NEWS for v1.5.0 names MIDI-CI's initiators, the Workbench runs and
  the seven QPlug tutorials.

## 2026-10-02

`a08f1e6c` and `59ca7ccb` fix the Windows `q_plug` CI jobs. With QPlug on,
Q now builds with the static MSVC runtime throughout, so q_io's RtAudio
links with q_plug; RtAudio's CMake predates the runtime setting, hence the
CMP0091 policy default. And q_io clears the `CMAKE_DEBUG_POSTFIX d` RtAudio
leaves in the cache, which renamed every Debug library after it, the
plugin modules included. Both verified on Windows, then on CI.

`60a03f9b` `profile_initiator` and `property_initiator`, the asking side
of profiles and properties. The first keeps no table: `ask`, `turn_on`
and `turn_off`, with each profile a reply or report names reaching
`profile_state` and the end of an inquiry `profiles_listed`. The second
holds four requests in flight: `capabilities`, `get`, `set` (chunked to
the device's SysEx maximum), `subscribe` and `unsubscribe`; a chunked
reply is gathered up to 4096 bytes, an unanswered request expires as
`pe_reply::timed_out`, a NAK ends one as `pe_reply::refused`, and an
update for our subscription is answered with 200. Every initiator passes
on the others' hooks, so one application stage after the last hears all
of them, and `initiator` and `property_initiator` gained two argument
deduction guides (the one argument guide alone let a window argument
copy the chain). Live against JUCE's demo as a responder: its profile
listing and its `ResourceList` came back. `d7b49b7d` has the endpoint
example ask each device it finds; `6c4c95af` documents them; `b0adff05`
marks the item done on the conformance page.

`d7e8e980` `midi_ci::initiator`, the asking side of discovery. It sits in
front of the responder chain and shares its MUID (the profile and
property responders now forward `announce`, as they forward `muid`),
sends Discovery when a round starts, and keeps a table of
`remote_device`s from each Reply to Discovery and each Discovery another
device sends. An Invalidate MUID naming one removes it; a device that
misses a round's reply window is removed too; a reply carrying our own
MUID is a collision, answered by invalidating it so the responder takes
a new MUID and asks again. No clock: the caller passes the time, and
the default window is 3 s in nanoseconds. Hooks `device_added` and
`device_removed` on the next stage, a fixed table of 16. Thirteen cases
in `midi_ci_initiator`; live, it found JUCE's demo as a responder.
`ae89783d` has the endpoint example ask every 30 seconds and print the list.
`9e2d7a32` documents it, `61b0caea` marks it done on the conformance page; asking
a found device for its profiles and properties is the next open item.

`dd1d1949` On Linux, ALSA translates a MIDI 1.0 voice message sent to a
MIDI 2.0 client into MIDI 2.0, scaled as M2-115 does, so
`midi2_loopback` now expects that there and the untouched packet
elsewhere; the q_io MIDI 2.0 stream page says so. Turning ALSA's
conversion off was considered and dropped: it also stops the conversion
of events from MIDI 1.0 programs, which libremidi's input would then
misread. `50d99a45` adds the Linux run to the conformance page: Ubuntu 26.04,
gcc 15.2, 104 of 104, the MIDI loopbacks running over ALSA virtual
ports rather than skipping as they do on Windows.

`a9e80265` The conformance page opens with a checklist, done and still to do.
The whole suite was built on Windows 10 with Visual Studio 2022, 64 bit
(`build-x64` on the Windows box, from a bundle of develop): 103 of 104,
every MIDI test passing. The tests that open a virtual MIDI port skip
there, and `test_audio_stream` fails over SSH, where the output callback
never runs (0 calls); neither says anything about the code, but the
device side on Windows is untested.

`9e06d31e` The endpoint example's `X-Gain` can be subscribed to, through Q's
`subscribe`, `unsubscribe` and `muid_invalidated` hooks, and each Set
sends its subscribers the new value; a fourth profile sits on channel 1.
Run against JUCE's initiator: subscribe, update, unsubscribe, and an
Invalidate MUID ending a subscription all behaved, as did a profile
inquiry and Set Profile On and Off at a channel. `85e2f799` updates the
conformance page to match.

`01700c38` A public MIDI 2.0 Conformance page under the MIDI reference: the
clause tests, the Workbench's automated and manual checklist items, the
JUCE and Logic Pro runs, what a device built on Q supplies itself, and
what is not yet tested (Windows, Linux, hardware, Q as an initiator).

`0902462b` The endpoint example hands its product instance id to the MIDI-CI
responder too, so an Endpoint Inquiry gets "Q-0001" rather than an empty
reply. Found running JUCE's CapabilityInquiryDemo against it, the first
MIDI-CI initiator not written here: discovery, profiles on and off,
property Get and Set (a 1202 byte Set gathered from three chunks) and a
refused subscription all behaved.

`cb8de2c1` libremidi is pinned to Cycfi's fork, `cycfi/libremidi` branch
`q-pin`: upstream `9d69bfb` plus one fix. libremidi gave the reserved UMP
message types no size, so a single such packet (a 128 bit type 0xE, sent
by the MIDI 2.0 Workbench) crashed q_io's input on the CoreMIDI thread
before q saw it. The fix sizes every type and stops at a packet cut short
by the buffer; it is offered upstream as `reserved-ump-sizes`, and q goes
back to celtera once it lands. `cb033405` writes the endpoint example's
manufacturer id `0x7D0000`, a one byte id in the first byte, as Q's
identity expects.

`ec9cc1b8` `property_responder` gathers a property exchange Set sent in
chunks (M2-103 8.3) before calling `set_property`, once, with the whole
body. Before, each chunk was taken as a whole Set, and every chunk after
the first had no resource name. Found working the MIDI 2.0 Workbench's
manual checklist (PE2.1 to PE2.3). The buffer is a fourth template
parameter, `SetCapacity`, 4096 by default; a longer body is refused whole
with 413, a chunk numbered zero ends the Set with nothing taken, one Set
is gathered at a time (another gets 343), and an Invalidate MUID naming
the sender abandons it. `435538a0` adds the missing test for PF3.6.
`3618f5e3` gives `example/midi2_endpoint` two permanent profiles and a
settable `X-Gain` resource, so the checklist has something to switch and
set.

`bc6b6a61` Dexter, the last QPlug tutorial: a six operator FM synth that
plays DX7 patches, with all 145 voice fields as parameters, ROM1A as the
factory presets and `.syx` cartridges dropped on the editor as user
presets. The editor is built from stock Elements parts that this work
added to Elements (`curve_editor`, `image_grid_menu`, `image_regions`,
`curve_lines` shapes, `image::fill`, `button_body`, `button_face`); the
pin is at `a74c5176`.

In q: `dx_cartridge` and `dx_voice` decode DX7 sysex dumps (a bad
checksum is not grounds to refuse). `fm_voice` gains a mod wheel,
per-operator switches and `update`, which gives a sounding note a new
patch without restarting it, as a DX7 does on a patch change.

In QPlug: `controller::add_presets` adds many user presets in one write,
presets keep their files' order, `dont_save` parameters stay in the
session, and `processor::parameters_changed` is called once per run of
frames when a parameter has changed. A `live` parameter, read directly as
the processor plays, does not count. The VA Synth examples and tutorials
are now Anna I to V.

## 2026-09-29

`8d176764` q is MIT again, as it was until the BSL-1.0 relicense of
2026-05-06. BSL bought nothing: q_lib already includes MIT infra, and q_io
and QPlug users owe notices anyway.

`64f8b1a3` to `a5cdb09d` QPlug is folded into q as `q_plug`, a component
beside `q_lib` and `q_io`, built with `-DQ_BUILD_PLUG=ON` (off by default).
Its history came in whole: the qplug repo was rewritten into the q layout
and merged, so `git log --follow` reaches back through it. The namespace is
`cycfi::q_plug`; the plugin IDs stay `com.qplug.*`. Elements, CLAP,
clap-wrapper (from free-audio, not the fork) and nlohmann json are fetched
only when the option is on, and a path-triggered `q_plug.yml` workflow
builds it so the main Build never fetches them. Along the way: infra to
`62a88ca`, the one Elements needs; q_plug ported to today's q API; its docs
join the site as their own module; the layers figure is generated; Q_IO is
called QIO in prose.

## 2026-09-28

`5ba7ce3b`, `313d8126`, `11e54884`, `18358a29` The `midi_2` branch landed on
develop as four commits: the rest of MIDI 2.0, q_io's move to RtAudio with a
MIDI output stream, the fixes of 2026-09-27, and the reference pages. Beyond
the reading side logged on 2026-09-10, MIDI 2.0 became bidirectional:
`Source` and `Sink` concepts, with every responder answering into a sink the
caller provides.

`9c2dbea2`, `d1c3f21a` The virtual analog parts landed, and an example and a
tutorial for each of VA and FM. The parts are `basic_ladder<Cell, N>`, with
the cell and the pole count as parameters, `analog_osc`, a symmetry control
on the triangle oscillators, and `hard_sync`. They came from an attempt to
emulate the Prophet-5 and the Oberheim, dropped the same day; what stays is
general DSP. `fast_tanh` now clamps its argument to ±9, where tanh is already
1 in float, so a large input no longer yields NaN.

`928006ef` Standard MIDI files. `midi::file_reader` plays a file into a
processor, the same one a live input feeds, timed in samples, with tempo
changes applied and tracks merged in time order. q_io's `midi_file` reads one
from disk. Formats 0 and 1 play as one piece; format 2's tracks play one
after another.

`d88dd825`, `eafc06bc`, `13958de0` The docs open figures in a lightbox,
generate their block diagrams from a script, and had their prose tidied.

`f352582e`, `5ad859c1`, `91c925e1` A branching model: develop is the main
line, and master only fast-forwards to the newest release branch. master and
`v1.0` were merged into develop, changing no files but the site trigger, so
master can fast-forward at 1.5. The releases are tagged `v1.0.0` (the
2024-08-21 release) and `v1.0.1` (`v1.0`'s head). The site publishes only
from develop and `v*`, one deploy at a time: pushing the tags had run
`v1.0`'s own workflow, whose playbook builds master, and its deploy replaced
develop's.

`924ad6df`, `308ddccb` Two portability fixes CI found once the branches
landed; Clang accepted both. `fm_voice`'s constrained members, defined outside
the class, used a constraint GCC and MSVC could not match to the declaration;
they are defined in the class now. And six braced numbers building a MIDI 2.0
`note_on` or `note_off` were ambiguous, because each inherited a protected
six-argument builder; each now declares only the packet constructor it needs.

## 2026-09-27

`fced5e22`, `e56c5387`, `510772c6` FM synthesis, and a DX7 that plays the
factory patches. Two layers: `q/synth/fm/` is a plain FM synthesizer in ideal
units (operators, a routing expression, an algorithm runner, a voice), and
`dx_patcher` compiles a DX7 patch's 0 to 99 counts into ratios, decibels and
seconds, with the key and velocity scalings precomputed so a note-on is table
reads. Nothing below the patcher exists because of the DX7. The 32 algorithms
are routing expressions (`op<2> >> op<1> | op<6> >> op<5>`), which caught two
wrong rows the hex masks had. The laws come from the MSA measurements and Ken
Shirriff's reverse engineering, and the rest was measured on Dexed: the
factory patches land 0.7 to 2.1 dB from Dexed in frame RMS envelope distance.
The VA oscillators moved to `q/synth/va/` and the generators to
`q/synth/gen/`.

`313d8126`, `18358a29` q_io's audio moved from PortAudio to RtAudio 6.0.1,
pinned. With the stream test's own race fixed, fifty runs showed PortAudio
hanging 3 times in `Pa_StopStream`: a lock-order deadlock in its CoreAudio
backend, which RtAudio's CoreAudio path avoids. miniaudio, the alternative,
drives CoreAudio the same way as PortAudio and has no ASIO. The API held, plus
`audio_device::default_id`; four behaviours moved (frames, `time()`, duplex
latency, disconnect), all described on the q_io reference pages.

`11e54884` Two bugs, found by checking every public name against the
reference. `rt_exp_moving_average::width` left `b_` at the old span, so after
a width change a steady 1.0 settled at 0.109. `one_shot_phase_iterator`
inherited `begin()`, `end()` and `middle()`, which returned a wrapping
`phase_iterator`. Nothing in q or hz used either.

`11e54884`, `18358a29` The names that sweep found missing from the reference
are documented, with figures from generators checked against the headers, and
tested where nothing tested them; `sample_hold`, the delta gates and
`spectral_flux` have pages of their own, and `sample_hold.hpp` compiles on its
own. `leaky_integrator` is deprecated: it is `one_pole_lowpass` with an
approximate pole, 7% high at 1 kHz and negative above `sps / 2 pi`.
`fixed_pt_leaky_integrator` stays.

## 2026-09-26

`20e46fb6` Moving sums and averages take their accumulator type as a
parameter, `double` for a `float` sum by default. A `float` accumulator, for
a Cortex-M4F without double hardware, re-sums every window to bound its
drift: within 0.003 of `double` over 2 million samples. `cubic_clip`
computes in `float`.

## 2026-09-17

`d1739a36`, `3420f7d4` What both MIDI protocols share has a namespace of its
own, `q::midi`: the processor concept, `message_base`, the no-op processor,
`cc`, the note helpers and the per-note messages. Each protocol's namespace
still names what it uses, so nothing broke.

## 2026-09-10 (2)

`edc4d4a4`, `a849317f`, `368b3a8f`, `71e60f6f`, `e8f1b7d8`, `7588800d`,
`c11abed9` A MIDI 2.0 endpoint a host can discover, the part the MIDI
Association's conformance tool tests. The messages derive from
`packet_message<Words>`, the counterpart of `message<N>`. The UMP 1.1 stream
messages are read, a `stream_responder` answers endpoint and function block
discovery from an `endpoint_description`, and a MIDI-CI `responder` answers
discovery under the specification's MUID rules. A loopback test runs the
whole stack through CoreMIDI, and found two libremidi problems: a stack
overrun on the first stream packet, fixed upstream and pinned, and sysex
dropped by default.

`8f117a38`, `20bbebb1` q_io gained packet streams, `midi2_input_stream` and
`midi2_output_stream`, so nothing above q_io names libremidi. They turn off
libremidi's rewriting of MIDI 1.0 packets as 2.0, since translation is the
program's choice.

## 2026-09-10

`6a516500`, `b2fa3f3a`, `d3216679`, `2e6285c4`, `da9166ae` MIDI 2.0 reads,
in `q::midi_2_0`: Universal MIDI Packets and the voice messages, in the same
shape as MIDI 1.0's; resolution scaling to the M2-115-U tables, with 8192
landing on 0x80000000 so an idle wheel never detunes; translation both ways
per Appendix D, as stages that wrap a processor; sysex gathered from packets
into the same `sysex_view` the byte reader gives; and a per-note reader that
turns per-note bend, pressure and controller 74 into the MPE note messages,
so one synth hears either. Every test is named for its specification clause.

`f0f47f03`, `d5ec4c4e`, `7798932c` The envelope: a ramp keeps running when its
width changes, the sustain follows the config's shape (a sustain rate runs it
down, none holds it), and the sustain level can move after the envelope is
built.

## 2026-09-09 (3)

`f0fbf240`, `2699296e`, `19a3caa5` MPE reads: a zone's channel messages arrive
as `note_pitch`, `note_pressure` and `note_timbre` for the note they belong
to. The first cut, written from a reading of how MPE works, was wrong in six
places, which RP-053 exposed once each normative clause became a named test.
The deepest: a member channel holds several notes once a zone runs out of
channels, so a channel does not identify a note. Where the specification
says only "combine meaningfully", the choices are in
`docs/mpe_conformance.md`.

## 2026-09-09 (2)

`6069a3ea`, `d160122e`, `8014f395`, `78ab1974`, `9bb04e2f` MIDI 1.0 is
finished and moves to `q/midi/`: readers for registered and unregistered
parameters, 14 bit controller pairs, a byte stream with sysex, and the channel
mode messages. Each reader wraps a processor and forwards what it does not
handle, so they chain, and the order matters: a controller reader in front of
a parameter reader would eat data entry. A coarse controller half reports at
once, since waiting for its fine half needs a clock; an oversize sysex is
dropped whole, never truncated. The sysex builder came from nexus.

## 2026-09-09

`77f67967`, `4a60456d` libremidi replaces PortMidi, whose packed three-byte
event cannot grow into MIDI 2.0. The API held. A lock-free queue joins
libremidi's callback thread to the program's loop, and a full queue drops the
new event, since dropping the oldest could lose a note-on whose note-off
follows. Timestamps are the host's own, in nanoseconds, and virtual ports are
listed again.

## 2026-09-02

`74276f9a`, `5996120a` `zero_crossing_ex` takes a hysteresis per call and
gains `reset`.

## 2026-09-01

`38e9c0d0` `mirrored_ring_buffer` stores every sample twice, so any window of
the history is one contiguous span, for kernels that want plain pointers.

## 2026-08-23

`00d229c9`, `ceb8d113` `sample_hold`, `delta_gate` and `spectral_flux`, small
blocks that compare a signal with its own recent past, for the hz onset
detector.

## 2026-08-22

`06b0a424`, `5d4dd1c9`, `42963035` The interpolation arithmetic is available
as free functions, joined by `peak_offset`, `zero_projection` and
`lagrange6_interpolate`.

## 2026-08-20

`b3629384`, `477c28f7` `fast_downsample` became `basic_fast_downsample<N>`,
binomial kernels of width 2 to 5, with the old name kept for the three-tap.
It serves the per-string pitch front end's four-stage decimation: binomial
kernels have no ripple to compound across stages, where a 9-tap halfband
tested worse, and four cascaded stages equal a CIC decimator exactly, without
its unbounded integrators. The integer result is now more accurate, since the
old kernel divided before summing. Reasoning: KB
`q/analytic-signals/why-fast-downsample-5.md`.

## 2026-08-12

`627145a8` The signal conditioner can bypass its clip and compressor at
compile time, for the hz comb, which reads the unclipped signal.

`76f40861` Non-owning followers, `moving_sum_ref`, `moving_average_ref` and
`true_rms_envelope_follower_ref`, compute over a history the caller owns, so
several windows can share one buffer.

`434a06f8` `concepts.hpp` and `basic_concepts.hpp` shared an include guard,
so whichever was included first silently emptied the other.

## 2026-08-03

`1180ed9b` The test and example CMake minimum rose from 3.5.1 to 3.16,
matching the rest of the tree.

## 2026-07-23

`1ef3a40b`, `4309b2e9`, `12ceb819` The `signal_conditioner` runs its dynamic
smoother before the pre-clip, and `smoothed()` exposes the signal there. Clip
and compressor had been biasing peak timing, making span-based period
estimates 1 to 3% flat on the hz corpus. The pre-clip is now `tanh_clip`, with
`hard_clip`'s rail interface. Five golden suites drifted 0.2 to 0.4% of rows
and were re-minted.

`cb6dd60e`, `1828cd3d` The peak picker's real-audio test and figures read
`smoothed()`.

`0a9514b2`, `8c3ecb2d` The golden compare allows small mismatches in 0.1% of
rows (at least one), with a gross-deviation guard, because `fast_tanh`
differs across architectures. A Rosetta x86 build reproduces the x86 side
locally.

## 2026-07-21

`00a1cc23` `q::peak_picker`, a causal local-maximum picker from the sign
change of the first difference, reporting the exact apex one sample late with
no envelope to tune. Selectivity comes from qualifiers composed around it
(`peak_gate`, `peak_min_slope`, `peak_z_score`), which take the running level
from the caller rather than build their own.

`5307ce05` `magspec` read the wrong bins: it assumed a split layout, where
`fft` stores interleaved pairs. Fixed, with an FFT test suite the old code
fails.

## 2026-07-19

`650d04c5` `note_number()` returns -1 for a letter outside A to G, where it
read uninitialized memory.

`0e1bebc1` System common and real-time messages reach the processor again; an
old fix had swallowed all nine since the processor was added. (Ray Chern, #94)

`cdb09037`, `461778ea` More MIDI dispatch and note tests, and Linux CI
installs the ALSA headers.

## 2026-07-13

`8b24fc65` The onset gate's level and slope thresholds are separate, so a
loud note opens it at once while slow creeps stay rejected.

## 2026-07-12

`b72fcf7d` A manifest of the test audio records each file's open-string
frequency.

## 2026-07-09

`1babefb8` A missing space in the test build's compiler flags, harmless until
a user passed flags of their own.

`59811a1a` Resonant filters can be placed by frequency and decay time.

## 2026-07-05

`caa69455`, `647a407d` State-variable resonant filters: a modulation-safe SVF,
a Moog ladder, and `reso_filter` rebuilt as a Chamberlin filter under its own
name.

`4aa1631c` The envelope retriggers from any phase, with stress tests on a
weekly CI job.

`22122aa3`, `221d18e0` `one_pole_lowpass` uses `fast_exp`, and the
`poly_synth` example and tutorial.

## 2026-06-20

`06026735` Cycfi's own dependencies come from submodules with a fetch
fallback, and third-party ones are fetched. The install guide was rewritten,
and a CI job runs its steps on all three platforms.

`8a0b03d6` The Waveforms example and tutorial.

## 2026-06-18

`cb9c0ec3` Goldens in per-test directories, under one comparison mechanism.

`1eaa6840` `fast_sqrt` forwards to `std::sqrt`, which measured both faster and
exact.

## 2026-06-17

`552f5b09` Mains hum removed from twelve low-note test samples, their attacks
preserved.

## 2026-06-16

`f6d9c498` The clippers renamed `hard_clip`, `cubic_clip` and `tanh_clip`, the
old names deprecated, with a new `fast_tanh`.

## 2026-06-15

`aa221503` A rational tanh saturator, `soft_clip2`, later folded into
`tanh_clip`.

## 2026-06-14

`39edd06b` `period_detector` renamed `bacf_period_detector`, the old name
deprecated.

`0525d9fe` Benchmarks build without being registered as tests.

## 2026-06-13

`db6a4ec0` Each example in its own directory. `b06e545d` CI skips docs-only
changes.

## 2026-06-12

`0fb82dd2` A true RMS follower that measures power. The fast follower reads
peaks, which was turning bass crest wobble into gain pumping in hz's level
matching.

`1763ff10`, `1b75567a` Contract tests for every envelope follower.
`2bcda352` `fast_rms` exposes `peak_square()`.

`ad43e407`, `4f2477ef` Golden CSVs, quiet under CI, and level goldens that
tolerate the spread between platforms.

`93f4cbf1` Window generators retune and restart in one call. `0b9c07de`
Sustain hold's hand-over ramp no longer plays backwards on every other
engage.

`a797b823`, `9aafcb88` CI actions moved to Node 24.

## 2026-06-11

`f1adef8a` The first PSOLA blocks: interpolation policies, a compact sine
table, the `grain` primitive, `best_lag`, and the grain freeze and sustain
hold examples. `1c9b443d`, `1ac69dd4`, `cb32a98b` Sustain hold runs on
Windows.

## 2026-06-10

`d9212e72` The exact log2(10) in `fast_pow10`, removing a bias at no cost.

`f37f5379` `monostable`'s pulse length can change at runtime, so hz's onset
gate can hold for one cycle of the tracked pitch.

`a34b21b6` Windows CI pinned to windows-2022, after windows-latest moved to
VS2026. (#93)
