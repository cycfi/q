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

## 2026-11-05

`173cf337`: the pitch shifting pages and the BACF Lag Tracker page play
the blog's final 16:9 videos, `rotating_head_v2.mp4` and
`chord_strobe_v2.mp4`; the lag tracker's own video was never uploaded to
R2, and its caption now quotes the strobe video's numbers. `ad82ffc0`: the
doc images no page used (11) and the generators that made only them (4)
removed.

`59cd11a3`: `q::midi::bend_range`, a pitch bend as semitones or a ratio,
for MIDI 1.0 and 2.0, with its own depth down and up. Three straight
segments: a middle of `linear` semitones either way (default 1) at the
mean of the two rates, so a vibrato across the center stays even, then
down and up to their depths. `example/whammy` uses it. First step of the
Q Bender plugin.

## 2026-11-04

On `sola-shift`: `630627d6`, `pitch_detector` renamed `bacf_pitch_detector`
after its algorithm, beside `bacf_period_detector`; the old name stays as a
deprecated alias in `<q/pitch/pitch_detector.hpp>`. `sola_pitch_shifter`
renamed `bacf_pitch_shifter`, with no alias (it was never released).

`b1b130a2`: the pitch detection and pitch shifting docs rewritten from
Joel's blog posts, with figures from the guitar test samples. New BACF Lag
Tracker page. Pitch Shifting now opens with the methods and their history
(rotating head, ring buffer); the shifter page explains splicing by the
period, finding it, overlap-add and picks; the PSOLA against SOLA formant
explanation moved to Formant Preservation. Measured for the record: the
BACF pitch detector costs about 4 to 5 ns a sample on an M2 (18 single
notes and 2 phrases), the same as the lag tracker.

## 2026-11-03

`6bc20be6`, `7a76878e`: `decibel` is a float unit, and `basic_decibel<T>`
with the dynamics templates of `92fa1e4d` is gone: nothing needed a
double decibel (the conversions are float tables, a float holds a level
to about 1e-5 dB, and nothing accumulates decibels), so the compressor,
soft knee compressor, expander, agc, level crossfade and signal
conditioner are plain classes again, in hardware float on a Cortex-M4F.
The RMS followers' T is their accumulator only. Suites 124 of 124 on
ARM and x86; hz compiles against it.

`73a1b86f` to `a6f8aea3`: every class that keeps double state or
arithmetic per sample is a `basic_` template on T, its old name the
double form, so a single-precision FPU instantiates it in float:
`basic_decibel<T>` (`dB<T>`, `lin_to_db<T>`, `approx_db<T>`; the two
forms mix and convert) and on it the compressor, soft knee compressor,
expander, agc, level crossfade and the signal conditioner's second
parameter; the Hamming, Blackman and exponential generators; the RMS
followers (T is the accumulator and the `_db` decibel) and
`moving_average_ref`'s accumulator; `basic_dx_envelope_gen` and
`basic_fm_operator`'s ratio type. Stray double literals in `saw_osc`,
`poly_blamp` (it returned double) and `zero_crossing_collector` are float.
On a Cortex-M4F every per-sample path in float has no software double;
the suites pass as they were, 124 of 124 on ARM and x86. The anna examples read the now double
`sin_cos_gen` as float. Docs: the template forms, `sin_cos_gen`'s
precision, and the mirror length.

On `sola-shift`: `a9a0bf65`, the SOLA shifter's engaged latency, after
Katja's low-latency splice (takes 51 and 52, KB `e_whammy/renders`).
Going down it splices forward by as many periods as fit, to just behind
the input; going up the read keeps only the headroom the next fade
needs. Each period splice refines the tracked lag to the full band
within 4 samples; the lag is smoothed as the period detectors do (set
outright on a lock or a jump, else a median of 3 and an EMA), which
keeps the tracker's one-edge outliers out of the splices; fades run
from 2 ms. Mean read delay, single notes and lines: 2 to 11 ms down
(was 31 to 39), 2 to 8 ms up (was 4 to 9); chords 12 to 20 ms, bound by
their 24 ms period. F446 at 48 kHz: 720 to 820 cycles a sample, worst
block 450 us (68%), from about 4k multiply-adds a splice. New test: a
change comes out within 10 ms at 0.5 to 2.

`84780898`: `basic_sin_cos_gen<T>`, `basic_hann_gen<T>` and the basic Hann
ramps, with the old names as their double forms. The oscillator's state
is now T as well (it was float under a double coefficient, which bought
nothing: float and the old mix both hold the peak within 1e-6 however
long they run, double within 1e-8). Float is for a single-precision FPU
where the run is short, such as a crossfade.

`1a8c4639`: `mirrored_ring_buffer(size, mirror)` repeats only the first
`mirror` elements after the end, so windows of up to mirror + 1 elements
stay contiguous at that many extra elements instead of a doubled store.
The default mirror is the size, the old behavior; reads now wrap their
start with one compare. A 4-point interpolated read wants a mirror of 3.

On `sola-shift`: `88a755a7`, `varispeed_tap` fades with a float Hann ramp,
which runs at most a fade from a reset; the shifter's output moves by
-138 dB at most. On the F446 the per-sample software double is gone: the
worst block drops from about 55k to 39k cycles (306 to 216 us), the mean
at +12 from 790 to 640 cycles a sample.

On `sola-shift`: `7e95b08c`, `varispeed_tap` takes its delay line as a
`Buffer` policy, by default a `mirrored_ring_buffer` with a mirror of 3,
and `history()` is gone. The SOLA shifter's delay line halves, 92 to
46 KB at 48 kHz, so two fit an F446; its output is bit-identical, and on
the F446 its cost is unchanged (660 to 800 cycles a sample, worst block
about 55k cycles).

On `sola-shift`: `9f7e9e9e`, the SOLA shifter finds its lag with
`bacf_lag_tracker` (the input through a 1 kHz low-pass) and splices at
it, fraction included, with no float refine at the read; at a transient
the lock drops and the last lag stands in until a new one. Its output
equals take 50's approved `__bacf_lite` exactly with 32-bit words (the
H723's). On the H723 it costs 0.64 to 0.68 us a sample (3%), worst block
33 to 35 us (5%), against 2.6 us and 276 to 325 us before. `lag_search`,
`lag_tracker` and `lag_bank` are no longer used by it. `3dd5b847` (on
`bacf-lag-tracker`): the tracker rounds its lag range to whole samples.
`a27ff833`: `lag_search`, `lag_tracker`, `lag_bank` and their kernels
(`q/detail/lag_kernels.hpp`) removed, and `best_lag` back to develop's
plain search (`sustain_hold` uses it); the SOLA shifter stays in q.

`1f60b3b2`: `bitstream_acf` counts its mismatches with Harley-Seal where
the target has no hardware popcount (a Cortex-M, baseline x86-64):
carry-save adders sum eight XORed words bitwise and one count serves the
eight; the shifted word is loaded once and carried. On the H723 a 16-word
window went from 22 to 14.5 cycles a word; `bacf_period_detector`'s
cueing runs 1.5x faster at a 45 ms window (G's worst detection 3.1 to
2.0 ms), 1.1 to 1.2x at a guitar pitch window. NEON, POPCNT and MSVC keep
the one-a-word count. The counts are unchanged.

## 2026-11-02

On `bacf-lag-tracker`: `2f3be574`, `bitstream_acf` correlates any window
of its bits (a start word and a length). `188b92e7`, `bacf_lag_tracker`:
a lag tracked by bitstream autocorrelation, one correlation per zero
crossing, leading and trailing: the locked lag swept two samples either
side over an 11.6 ms window, a parabola for the fraction, twice the lag
tried on a miss (a chord's common period), the last good lag held through
a loss; acquisition from the latest similar pulses. Octaves are left
unsettled, which a splice does not mind. On the guitar single notes it
holds the period from 0.2 s after the pick, locked on it 99 to 100% of the
time. As the SOLA shifter's lag source (scratch, take 50, splices at its
lag with no float refine) the shifter costs 0.65 us a sample on the H723,
worst block 43 us (6%), against 2.6 us and 280 us with lag_search and
lag_tracker; Joel on the renders: "It's perfect!"

`aca757e1`, `63c12d05`, `83e2ceae`: the phase vocoder's per-bin math for
small cores, where a float compare and its flag transfer cost far more than
the arithmetic. `fast_sincos` reduces to the nearest quarter turn and picks
the quadrant by bit masks (error 9e-8); `fast_atan2` picks its octant from
sign bits with fmin and fmax (error 1.2e-5; `fast_atan2(0, -0.0)` is now
pi, as IEEE has it); `principal_arg` clamps with fmin and fmax.
`spectral_peaks` compares magnitudes as integers, which order as
non-negative floats do. `phase_tracker` gains `step()` and `next_frame()`,
so a caller can fold the tracking into a pass of its own over the bins.

`eee24034`: the native FFT at half the cost on a Cortex-M7. Its radix-2
stages run two to a pass as radix-4 butterflies (three twiddle multiplies
for four points), the first pass (twiddles 1 and -i) with none; the bit
reversal is a list of swaps; the real FFT's split takes w/2 from its table
and the inverse folds its 1/n into the split. On the H723 a 2048-point real
FFT went from 161k cycles to 79k, ahead of PFFFT's scalar code (91k) and
CMSIS-DSP 1.6.0's arm_rfft_fast_f32 (94k); 8192 points from 832k to 444k.
On the M2 the native path is faster too (2048: 6.3 to 3.9 us), and float
stays on PFFFT there. `b5ebb0be`: the FFT page's Performance section (M2
and H723, Q native against PFFFT) and the standard performance NOTE in
common.adoc; Q native named as the microcontroller's choice.

On `sola-shift`, later the same day. `2c723db1`: the SOLA shifter's lag
range reaches 45 ms, so an open G chord (repeating every 41 ms) locks
instead of warbling. `61d6fd85`: its acquisition costs a flat amount a
sample. Timed in 32-sample blocks on the H723, the worst block ran 840 us
of its 667 (126%) about 0.1 s after a pick: a new lock's seeding summed
long double chains, the search did a whole window a sample, and the
tracker walked a window for its entering lag most samples. Now
`lag_search` budgets multiply-adds (256 a sample, a lag spanning calls),
seeding takes 32 positions a sample, the tracker moves at most once every
8 samples, and the scalar kernels (the MCU's) sum stretches of 32 in
float, in registers, into double: 8.4 cycles a sample for a correlation
against 14, 4.2 for a product against 10. NEON and SSE2 gained dedicated
product and square kernels in the same order, so their results hold.
Worst block 280 us (42%), none late; mean 2.6 us a sample. Renders in KB
`e_whammy/renders/49_sola_flat`, awaiting Joel's ear.

Formant preservation and the cores under it. `ff39e169`: `levinson` on
spans, its polynomial in place, and `lattice_fir`/`lattice_iir` at a
runtime order. `000b40e6`: `spectral_learner` (the long-term spectrum and
its spectral envelope), `lpc_fit` (an all-pole model; |A|^2 by Clenshaw
on a Cortex-M or by FFT elsewhere, a policy; the lags summed four ways)
and `eq_matcher` (B/A at no latency). `1186ef92`: `convolver`, partitioned
overlap-save behind a direct first block, the older partitions summed a
few a sample: 3.5 us a sample at 4096 taps on the H723, worst block 20%.
`b3890665`: `formant_learner` (a spectral learner fitting a
`formant_table` over +-24 semitones) and `formant_corrector` (an
eq_matcher client, its output bit-identical through that refactor); the
order is a runtime parameter, 32 by default. Joel on his four guitars
(take 47): "All of them sound really good"; on order 24 against 32 (take
48): both good, 32 more accurate. `7c205043`: the Pitch Shifting and
Frequency Domain reference groups, and the performance NOTE (48 kHz,
blocks of 32 samples, mean and worst block, M2 and H723).

## 2026-11-02 (earlier)

On `sola-shift`, to be squashed into develop. `sola_pitch_shifter`
(`0ad2b6c6`, rebuilt in `1b49e36b`): a time-domain pitch shifter with no
pitch detection, for the e-Whammy. A `varispeed_tap` reads the input at
the ratio; going up it splices back by a period, going down forward by
one, with Hann crossfades of half a period and no lag under 1.25
crossfades. The period is a `best_lag` match held by a `lag_tracker`
(`cd380682`): the correlation at seven adjacent lags as moving sums, so a
splice costs no search. Searches run through
`lag_search` (`a5fbcf5f`), best_lag's cued and coarse-to-fine searches
spread over samples within a budget (4096 multiply-adds a sample), cued by
the gaps between zero crossings whose hysteresis follows the envelope
(`9e7cab22`, a per-sample hysteresis and a capacity for
`zero_crossing_collector`; `222aed67`, an edge's delay). A
`transient_detector` (`a9247762`: spectral_flux into delta_gate, within
0.3 ms of a pick's release) restarts the read at the pick's own start:
going down at once, going up when its period is acquired, 13 to 18 ms on
single notes, 20 ms at the latest. `varispeed_tap` gained its splice and
reads at any delay (`af9211e3`), `best_lag` the coarse and cued forms
(`f4b7a9cd`). On the seven test samples at +-12 semitones: 0.23 to 0.37%
of an M2 core, no sample over 42 us, against 1 to 3% with 3 ms bursts
before the tracker. Joel auditioned the renders (KB
`e_whammy/renders/26_sola_streaming`). The doubles in the correlation sums
stay: the H723 target has a hardware double FPU.

`981d7fa1` `basic_lag_bank<Term>`: a sum at seven adjacent lags as eight
double lanes over a mirrored history, updated in one vector pass, with
seed and recentre; terms `lag_terms::product` and
`lag_terms::difference_square`, kernels for NEON, SSE2 and scalar in
`q/detail/lag_kernels.hpp`. It is the general core of a correlator
bank, so a difference-function bank can be its client. `66b90577` puts
the tap's delay line in a `mirrored_ring_buffer` (`history()`), whose
windows are contiguous spans; `f31ab195` runs best_lag's window sums on
the kernel when the buffer has spans; `3e4f04e6` rebuilds `lag_tracker`
on two banks and evaluates its similarities once a sample, which was its
real cost. Timed the same way before and after (each sample's minimum
over five runs), the shifter went from 84 to 100 ns a sample on an M2 to
27 to 39, its worst sample from 19 to 27 us to 6 to 7. Products are now
formed in float, which moves a few splices; Joel auditioned the renders
(KB `e_whammy/renders/28_sola_lag_bank`).

Sized for an MCU, after a bench on a NUCLEO-H723ZG and a NUCLEO-F446RE
(DWT cycles a sample) showed the M2-sized search budget and one-sample
walks. `adf5875c`: a `lag_tracker` restart seeds the new lock's sums over
the samples that follow, 64 positions a sample, exact (the window as it
stood at the restart), with `lag_bank`'s `restart` and `seed_part`.
`eeb9db27`: the shifter acquires its lag only when a strong pulse ends,
cued by the gaps from that edge to up to 16 before it, one correlation a
sample; a near miss (0.6 to 0.85) is probed 3 and 6% either side; no
coarse scan; a lock is searched again only once its match falls under
0.85, at most once a period; a forward splice's five-lag refine runs one
correlation a sample. H723 in double: mean 11.5% of the core (16%
before), worst sample 52 us (0.34 ms). M2: 24 to 39 ns a sample, worst
0.5 to 1.1 us. Renders KB `e_whammy/renders/33_sola_edge_acquisition`,
`34_sola_amortized`.

`73a87ca3`: unity bypass. The shifter starts at delay 0, and at ratio 1
the next pick returns the read to the input (a 1 ms crossfade), as does
silence, so with the bar at rest the output is the input, with no
latency; a pull-up splices from there. Renders KB
`e_whammy/renders/35_sola_unity_bypass`.

## 2026-10-31 (later)

`a0679ed9` breaking: the FFT is an object built with its size.
`q::fft` and `q::rfft` (`basic_fft<T>`, `basic_rfft<T>` for double)
replace `fft<N>`, `ifft<N>`, `rfft<N>`, `irfft<N>` and `magspec<N>`, so a
size can follow the sample rate and a program pays only for the sizes it
builds. Layouts and scaling are unchanged. An object owns its PFFFT setup
and aligned scratch, or Q's own tables: it allocates at construction, a
transform never does. Q's own FFT is now an iterative radix-2 with
precomputed twiddles, about 3x faster than the old template recursion
(2048-point complex frame 20.7 vs 62.7 us on an M2) and about 15x more
accurate in float. A generic `fft(n, data)` dispatch over 2 to 16384 was
tried first and dropped: it cost 100 KB of code and 640 KB at load. hz's
parked `dry_run` branch still calls `q::fft<N>`. Migration notes are on
the FFT page (`490aa709`) and in NEWS.

`0cd14b78` `principal_arg`, `fast_sincos` and `fast_atan2`. The wrap
subtracts 2 pi in two parts, 6.28125 (exact when multiplied) and the
remainder, and corrects a one-off k by a full turn: `fast_sincos` errs by
about 2e-7 to 100 rad, `fast_atan2` by 1.2e-5 (Abramowitz and Stegun
4.4.47), each about 3x faster than std.

`6451b4b5`, `5eab9a2e`, `f05a41e3` the STFT building blocks for qWhammy:
`stft_window` (Hann, and Mauler and Martin's low-delay pair, whose
latency is `delay`, not n), `stft` (one sample in, one out, a bin
callback a hop), `phase_tracker` (per-bin instantaneous frequency) and
`spectral_peaks` (Laroche and Dolson's peaks and the bins each owns).

## 2026-10-31

`143e6138` with `Q_USE_PFFFT` (a CMake option, on by default) the float
`fft`, `ifft`, `rfft` and `irfft` run on PFFFT, from 16 complex or 32 real
points up; `double` and smaller sizes stay native. PFFFT comes from Cycfi's
fork (cycfi/pffft `f5ce5e2`, three internal functions made `static`) and is
compiled header-only into `detail::pffft`, so libq still builds nothing.
The two builds live in different inline namespaces, so translation units
that disagree on the macro still link. On an M2, a 2048-point complex
frame (forward and inverse) went from 62.7 to 5.6 µs, a real one from 28.2
to 2.6 µs (`test/benchmark/fft_bench`). Accelerate was measured and
dropped: no faster than PFFFT for float once its split layout is converted
to Q's interleaved one.

`1fcc1beb` `rfft` and `irfft`: N real samples to the N/2+1 bins from DC to
Nyquist and back, in place in N+2 scalars, built on a half-size complex
FFT. Until now a real signal went through the full complex transform with
its imaginary slots zeroed.

## 2026-10-28

`3aa64210` `q::flush_denormals`, a scoped guard: it sets flush-to-zero on
this thread (MXCSR FTZ and DAZ on x86, FPCR.FZ on ARM64, FPSCR.FZ on 32-bit
ARM and Cortex-M) and restores the thread's own mode when it goes out of
scope, so a host's mode never leaks. QPlug's CLAP `process` and parameter
flush and q_io's audio callback hold one. Found when clap-validator's
parameter fuzz failed Ascend Pre: deep expander cuts and a closing gate left
subnormal output, and the validator rejects any. Subnormals are also slow
arithmetic on many processors, which is why the guard is the fix and not
snapping at the output; `q::snap` (#102) is for smoothers settling on their
targets.

## 2026-10-22

`08a145df` the decibel conversions split the number by exponent and mantissa,
the way a float is stored: `lin_float` reads 2^f from a 256-entry table and
the power of two from another, `lin_to_db` reads log2(1 + m) from a third
with the exponent field as the integer part. Both are exact to about 1e-5
dB over the whole float range (the old 0.1 dB table clamped at 120 dB and
was 8x coarser; the bit-trick log was 35x coarser), the tables are built at
compile time, 3 KB in all, and on the serial per-sample harness
`lin_to_db` is faster than `fast_log10` and `lin_float` level with the old
table. The RMS followers' floor drops from -60 to -120 dB RMS: a hiss floor
at -70 dB read as digital silence before, which the Ascend noise reducer
found. The `rms_envelope_follower` goldens are re-minted for the levels
below -60 dB that now read true. `test/support/decibel.cpp` sweeps both
conversions for accuracy, monotonicity across every table seam, the range
extremes, the constexpr path and the round trip, and prints the latencies.

`5b5aeded` QPlug takes Elements' `fit` (pin `61aa53b3`): an editor whose
content fits its view scales with its window. The CLAP resize hints carry
the view's aspect ratio and `adjust_size` keeps it; zoom resizes the window
to the declared size at the new zoom and reads the zoom back from the
view's scale, which `save_state` and `detach` store, so the editor reopens
at the size it was left. The zoom range narrows to 0.75 to 1.5 (from 0.5 to
2), which a plugin also passes to `fit` so resizing and zooming agree.

`d0f7d151` QPlug's Elements pin moves to Elements develop `4c823cf4`:
glowing fill tracks, the VU meter and pane styles, tooltips that switch off
together, an editable `combo_box`, an `led`, the outlined question mark
icon, and a fix for a crash when a layer under the focused one closes (a
window opened from a menu). Dexter follows the swatches rename:
`swatch_highlighter` and `region_highlighter`.

## 2026-10-05

`210e85a6` QPlug's byte streams are `byte_ostream` and `byte_istream`, no
longer `ostream` and `istream`, which read as the standard streams. They are
the two virtuals a controller saves and loads its state through.

`3cde8585` the v1.5.0 notes and the Highlights leave out onset detection,
which is Hz's to advertise, and the license, which no release changed.
`704f9d98` fixes four reference links that pointed at sections that do not
exist, and `53b48390` replaces the em dashes left in headers, tests and
scripts.

`e1a33e86` QPlug catches an exception from the editor at its CLAP GUI calls
(`create`, `set_parent`, `show` and the rest), logs it and returns false.
CLAP's C interface cannot carry an exception, and one that escaped ended the
host's process. Elements throws when it cannot make Skia's OpenGL context,
as where Windows offers only its OpenGL 1.1 (an SSH session, a CI runner
without a GPU).

`94e40d31` Windows links Skia and Cairo statically now (Artist `9334f33`,
through Elements `75dd7537`). The prebuilt bundles were DLLs, which Windows
does not look for beside a plugin, so the CLAP and VST3 failed to load.
`q_plug-backends.yml` tests Windows Skia and Cairo again, green with
pluginval's editor tests; Windows Skia gets Mesa's opengl32.dll beside the
validators. Advanced Builds' Current Limits drops its Windows row.

`b44d600d` QPlug pins clap-wrapper to Cycfi's fork (branch `q-pin`,
`af23b0f`): a standalone whose configured id matches no plugin in the CLAP
crashed on macOS (EXC_BAD_ACCESS in `doSetup`) and Linux. It now says so,
an alert on macOS and stderr on Linux, as Windows already did. Upstream
`next` still has it; the PR is to come (KB `q/midi/forks.md`).

`b3dcf8da`, `2b181017` the README shows Dexter after the Introduction.

## 2026-10-04

`c8ced5ab`, `bfd570c3` QPlug stops Elements' logging backend in the CLAP
entry's deinit, and when its last instance is destroyed. On Windows, process
exit kills threads before static destructors run, and quill's destructor
then waited on its dead thread: the validators hung at exit. A VST3 or AU
host may never call deinit, hence the instance count (Elements `230f95fb`,
`5c562499`).

`0aed7dde` a new QPlug instance starts on its first factory preset, unless
the host restores its state.

`81fbc0c6` QPlug takes its drawing backend from Elements' switches instead of
forcing one, and `e68b8068` `q_plug-backends.yml` tests the other backends
weekly; `6859254d` Linux Skia needs the OpenGL headers. `2c821360`,
`d65bd880` QPlug's tests run on Windows in CI; `310d4dec`, `7007561a` a hung
validation times out after 300 s, a Getting Started job after an hour.
`b4d3c8bb`, `f04a139e` the build options move to an Advanced Builds page,
and the standalone app and building outside Q get pages of their own.

`2ae134e3` the bindable_proxy tests declare their models before the binder,
which disconnects from them as it is destroyed: ASan caught it reaching a
model already gone.

`1461ae1e`, `d5106b58` CI draws its results per OS and backend as
`status/build.svg` on the www branch, shown in the README, from the latest
develop runs of build.yml, q_plug.yml and q_plug-backends.yml. `3df7bd2e`
`sanitize.yml` runs the tests weekly under ASan with UBSan (macOS, Linux,
Windows) and TSan (macOS, Linux); it leaves out the plugin validators and,
under TSan, QIO's device tests (KB `q/TODO.md`).

`f280cfd6` the tests move into directories that follow the include tree;
`79dafb45` removes a stray script. `ba6e8ced`, `a9e678e6`, `6168aecf`,
`e81d0928` the landing page carries the dependencies and the Discord link,
the README follows it, and NEWS gains Windows 11, onset detection and
microcontrollers; `e24bcd30` styles the navigation's expand button.
`15a022c2` updates Elements.

## 2026-10-03

`ebd88027` fixes the Endpoint Info Notification: Q set its MIDI 2.0
capability at bit 12, where M2-104 Figure 13 puts it at bit 9. Q's reader
used the same bit, so round trips passed, and macOS and the Workbench did
not object. Windows 11 did: it read no MIDI 2.0 support and set the stream
to MIDI 1.0. A new test checks the raw word against the figure, and two
parser tests that encoded the wrong bits are corrected. With aidi rebuilt
on it (aidi `d7a6c34`), Windows 11 26H2 binds the board to its in-box USB
MIDI 2.0 driver, runs endpoint and function block discovery, chooses the
MIDI 2.0 protocol, and gets a Reply to Discovery for MIDI-CI. `e21178c5`
records it on the conformance page.

`f1977271` the board passes the Workbench on Windows 11: every automated
check, profiles, property exchange and process inquiry, 0 errors. The
Workbench's bundled Windows helper was built against a withdrawn SDK
release candidate and carried no packets; our fork replaces it (KB
`q/midi/forks.md`). Nothing changed in Q's code.

`ca29e0e0` `remote_device::supports(category)`, true when a device
declared a MIDI-CI category in its reply. M2-101 v1.2 does not forbid
asking outside the declared categories (5.5.2; the device answers with a
NAK, 5.11), so the initiators still send what they are given and the check
is the caller's; the endpoint example uses it. `46299fd7` documents it.

`48826e25` M2-101 5.9.2: a second Reply to Discovery from one MUID in a round is
two devices sharing it, since a responder replies once to a Discovery. The
initiator now invalidates that MUID, passes the invalidation to the
responder chain (which ends anything held with it) and removes the device;
before, it took the second reply as the same device. A device that both
announces itself and replies is still one device. `dd66f148` quotes the
clauses in the initiator tests and corrects eight clause numbers (four
5.6.1 that were 5.9, four property-initiator sections one too high).

Release preparation for v1.5, docs only:

- `45bcf4d8`: the Highlights settled with Joel: six groups (Modern C++,
  Desktop to Microcontroller, MIDI 2.0, Signal Processing, Plugins and I/O,
  Learning), 29 lines, each linked to its exact section; the README's
  older one-line highlights dropped. Sections written for the lines that
  had none: `bb4ebb3c` Microcontrollers in Setup (tested on STM32H7 with
  Arm GCC 12.3), `7e01c6e3` Onset Detection on the Miscellaneous page,
  `c2ef3025` a MIDI File reference page for `midi_file` and
  `midi::file_reader`, which had no docs. `c06c96c9`: NEWS no longer calls
  the granular blocks a PSOLA family; PSOLA lives in Hz.

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

`ff89fdce` folds the Windows 11 Workbench run into the conformance
checklist. `90c33f41` the QPlug Setup page has one clone step for every
platform, and `3edc6263` CI runs the Setup and Testing pages when they
change; `09679d20` the validation scripts use the validators the build
downloaded, read from `CMakeCache.txt`.

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
