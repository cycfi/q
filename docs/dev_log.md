# q dev log

Internal. Dated, newest first, with commit hashes. Not published (lives outside
`modules/`, so the Antora build ignores it). Significant updates only. Narrative:
what changed and why, not how.

## 2026-07-19

`8aad25c1` note_number() could hand back garbage for anything that wasn't a real
note letter, reading uninitialized memory. It now returns -1 for a letter
outside A to G.

`d62b4be6` Widened the MIDI dispatch tests and added coverage for the note
utilities, closing gaps in the suite.

`b200fec7` The Linux CI build could not find ALSA. It now installs the audio
development headers so the build has what it needs.

`c5af2427` MIDI system common and real-time messages (song position, start,
stop, active sensing, and the rest) were never reaching the processor. An old
fix for channel messages had been quietly swallowing all nine of them ever since
the MIDI processor was added. They dispatch correctly now, with regression tests
to keep them from slipping back out. (Ray Chern, #94)

## 2026-07-13

`0b7eff2d` The onset gate had been forcing one threshold to stand for two
different things, a signal level and a rate of rise, so you could not set them
independently. They are separate controls now: a loud note opens the gate at
once without waiting on its attack, while slow creeps like noise and bleed are
still rejected. The signal conditioner adopts the new behavior.

## 2026-07-12

`f676a863` Added a manifest of the test audio files that records each sample's
open-string base frequency, so tests can look up the pitch a file actually
carries.

## 2026-07-09

`8649e981`, `c3ddf853` Fixed how the build assembled its compiler flags: a
missing space ran two flags together. Harmless until someone passed extra flags,
at which point it would have broken the build.

`1df44ea3` The resonant filters can now be placed the way a mode table actually
reads, by center frequency and decay time rather than by Q. New overloads take
the decay duration directly.

## 2026-07-05

`8397935f` one_pole_lowpass switched to the more accurate fast_exp for its
coefficient.

`056a256e` A new polyphonic sawtooth example, poly_synth, with a tutorial that
walks through building a voice-allocated synth from Q parts.

`7b9b0c8e` Fixed envelope retriggering. Restarting the attack used to work only
from idle; it now restarts cleanly from any point in the envelope. Added stress
tests and a weekly CI job to exercise them.

`62bfce36` Added a family of state-variable resonant filters: a
modulation-safe multimode SVF and a Moog-style ladder. The old reso_filter was
rebuilt as a proper Chamberlin filter and renamed, with the old name kept as a
deprecated alias.

## 2026-06-20

`2554dee9` A new example that plays Q's four band-limited oscillators (sawtooth,
square, pulse, triangle) in turn, with a tutorial page and a figure of the
shapes.

`a0854774` Reworked dependency management. Cycfi-owned infrastructure now comes
from its submodule so it can be edited in place, falling back to a fetch when
the submodule is absent so a plain clone still builds; third-party audio
libraries stay on fetch. The install guide was rewritten against a verified
build, and a new CI job runs the documented steps on all three platforms so the
instructions cannot silently rot.

## 2026-06-18

`49673e00` Tidied the golden test files into per-test subdirectories and put
every test on a single comparison mechanism, retiring the old exact-sample path.

`6b49760d` fast_sqrt now simply calls std::sqrt, which benchmarked as both the
fastest option on every target and exact, beating the old approximation. The
name stays as an alias so callers are untouched.

## 2026-06-17

`11fd720c` Twelve of the low-note guitar test samples carried baked-in 60 Hz
mains hum that was confusing the low-note detectors. They were cleaned offline
with the original attack preserved, and the affected lowpass goldens re-blessed.

## 2026-06-16

`10172cc4` Renamed the clipper family to intent-revealing names (hard, cubic,
tanh), keeping the old names as deprecated aliases, and added a fast_tanh. The
redundant tanh clippers collapsed into a single one built on it.

## 2026-06-15

`358c34c7` Added soft_clip2, a smooth tanh-shaped saturator. Where the cubic
soft clip flattens hard once past its knee, this one keeps rolling off
gradually, so louder inputs stay ordered instead of crushing onto the rail. Came
with tests and a benchmark.

## 2026-06-14

`d82698ba` Renamed period_detector to bacf_period_detector, naming it after its
algorithm and setting it apart from hz's correlator-based detector. The old name
stays as a deprecated alias.

`ca84ae32` Separated the pure microbenchmarks from the real tests so CI only
runs cases that assert something. The benchmarks still build, they are just not
registered as tests.

## 2026-06-13

`b45ad1ca` Restructured the examples so each lives in its own self-contained
directory with its own build file and audio, over a shared helper folder.

`5c891f61` CI now skips the build entirely for documentation-only changes.

## 2026-06-12

`fc96b65f` Made the level goldens survive across platforms. macOS, Ubuntu and
Windows produce slightly different floating-point dB levels and disagree even
among themselves, so the tolerance was widened to absorb the spread rather than
pinned to one platform.

`ce04b611`, `46d51132` Bumped the CI actions ahead of the Node 20 deprecation,
moving gh-pages, checkout and setup-node onto their Node 24 versions.

`e3015053` New golden-CSV test infrastructure, with guards that suppress WAV and
CSV output under CI, applied across all the real-audio tests.

`4e87dd54` Added a test that pins the fast follower's documented minimum hold,
turning the rule into something executable.

`1dc0413b` A contract test for every envelope follower, one per follower, each
pinning that follower's documented behavior: attack, decay, hold, and what it
actually reads.

`ef903e7b` Exposed peak_square() on fast_rms, the value under its square root,
so consumers working against squared thresholds can skip the sqrt. Added the
const getters the docs had already promised.

`99244542` Added a true RMS envelope follower that measures actual power, so two
signals of equal power read the same regardless of crest factor. The existing
fast follower reads peaks instead, which was turning bass crest wobble into an
audible gain pump in the hz PSOLA level matching; that job wants power, not
peaks.

`cc6970e7` The window generators can now retune and restart in a single call,
where config() previously only retuned.

`7b4b91ea` Fixed the dry hand-over crossfade in sustain hold: its ramp phase
carried over between engages, so every other engage played the ramp backwards.
Surfaced via the same bug downstream in hz.

## 2026-06-11

`6fdfd2fc` Stopped the Windows min/max macros from clashing with std under MSVC.

`6e61fd44` Added a small keyboard utility that hides the platform key-input
differences, collapsing the sustain_hold input loop into one platform-agnostic
block.

`0bb49492` Made the sustain_hold example run on Windows, replacing its POSIX-only
input path so it builds and runs everywhere.

`164fc43b` The first PSOLA building blocks: added interpolation policies and a
compact sine table, a windowed grain primitive and a normalized-correlation
best-lag search, plus a granular freeze and an interactive infinite-sustain
example.

## 2026-06-10

`d58b885c` Pinned the Windows CI runner to windows-2022. The latest image had
moved to VS2026, which broke the MSVC environment setup, so the build never even
reached compilation. (Joel de Guzman, #93)

`17f888ed` Improved the accuracy of the base-10 power approximations by using the
exact log2(10) constant, dropping a systematic bias at no added cost. Also
rewrote the decibel speed test to report honest throughput and latency instead
of a figure floored by the benchmark's own bookkeeping.

`e62f8779` monostable can now have its pulse length changed at runtime, so the
hz onset detector can hold its gate open for exactly one cycle of the pitch it
is currently tracking.
