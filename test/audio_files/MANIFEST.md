# Test audio manifest

Base frequency (and identity) for every sample in this folder, so tests don't
have to rediscover it. **`base_freq` is the OPEN-STRING frequency** (the lowest
note the string can sound), which is what the pitch/period/onset detectors want
for sizing their period range (base_freq up to base_freq + range). It is NOT the
played pitch; a fretted note sits above the base.

Naming: `N{a,b,c}-<String>`: `a` = open, `b` = 12th fret (+1 octave played),
`c` = 24th fret (+2 octaves played); the base frequency is the open string in
all three. Named samples (`GStaccato`, `Tapping D`, …) take the base of the
string they were played on.

In the C++ tests these come from `notes::` in `pitch.hpp` (`low_e = E[2]`, etc.).

## Strings

| var       | note | base (Hz) |
|-----------|------|-----------|
| `low_fs`  | F#1  | 46.25     |
| `low_b`   | B1   | 61.74     |
| `low_e`   | E2   | 82.41     |
| `a`       | A2   | 110.00    |
| `d`       | D3   | 146.83    |
| `g`       | G3   | 196.00    |
| `b`       | B3   | 246.94    |
| `high_e`  | E4   | 329.63    |

## Samples

| sample                | string   | base (Hz) | kind                     |
|-----------------------|----------|-----------|--------------------------|
| `-2a-F#`              | `low_fs` | 46.25     | open string              |
| `-2b-F#-12th`         | `low_fs` | 46.25     | 12th fret                |
| `-2c-F#-24th`         | `low_fs` | 46.25     | 24th fret                |
| `-1a-Low-B`           | `low_b`  | 61.74     | open string              |
| `-1b-Low-B-12th`      | `low_b`  | 61.74     | 12th fret                |
| `-1c-Low-B-24th`      | `low_b`  | 61.74     | 24th fret                |
| `1a-Low-E`            | `low_e`  | 82.41     | open string              |
| `1b-Low-E-12th`       | `low_e`  | 82.41     | 12th fret                |
| `1c-Low-E-24th`       | `low_e`  | 82.41     | 24th fret                |
| `2a-A`                | `a`      | 110.00    | open string              |
| `2b-A-12th`           | `a`      | 110.00    | 12th fret                |
| `2c-A-24th`           | `a`      | 110.00    | 24th fret                |
| `3a-D`                | `d`      | 146.83    | open string              |
| `3b-D-12th`           | `d`      | 146.83    | 12th fret                |
| `3c-D-24th`           | `d`      | 146.83    | 24th fret                |
| `4a-G`                | `g`      | 196.00    | open string              |
| `4b-G-12th`           | `g`      | 196.00    | 12th fret                |
| `4c-G-24th`           | `g`      | 196.00    | 24th fret                |
| `5a-B`                | `b`      | 246.94    | open string              |
| `5b-B-12th`           | `b`      | 246.94    | 12th fret                |
| `5c-B-24th`           | `b`      | 246.94    | 24th fret                |
| `6a-High-E`           | `high_e` | 329.63    | open string              |
| `6b-High-E-12th`      | `high_e` | 329.63    | 12th fret                |
| `6c-High-E-24th`      | `high_e` | 329.63    | 24th fret                |
| `Attack-Reset`        | `g`      | 196.00    | attack / re-articulation |
| `Bend-Slide G`        | `g`      | 196.00    | bend + slide             |
| `GLines1`             | `g`      | 196.00    | melodic line             |
| `GLines2`             | `g`      | 196.00    | melodic line             |
| `GLines3`             | `g`      | 196.00    | melodic line             |
| `GStaccato`           | `g`      | 196.00    | staccato                 |
| `SingleStaccato`      | `g`      | 196.00    | staccato (single note)   |
| `ShortStaccato`       | `g`      | 196.00    | staccato (short)         |
| `Slide G`             | `g`      | 196.00    | slide                    |
| `Slide Low E`         | `low_e`  | 82.41     | slide                    |
| `Tapping D`           | `d`      | 146.83    | tapping                  |
| `Hammer-Pull High E`  | `high_e` | 329.63    | hammer-on / pull-off     |
| `harmonic Low E`      | `low_e`  | 82.41     | natural harmonic         |
| `harmonic D`          | `d`      | 146.83    | natural harmonic         |
| `harmonic G`          | `g`      | 196.00    | natural harmonic         |
| `gk Low E`            | `low_e`  | 82.41     | pickup sample            |
| `sweep Low E`         | `low_e`  | 82.41     | pitch sweep              |
| `sin_440`             | `d`      | 146.83    | synthetic tone (440 Hz)  |
| `sin_envelope`        | `a`      | 110.00    | synthetic envelope tone  |
| `harmonics_261`       | n/a      | ~261      | harmonic content (C4)    |
| `harmonics_329`       | n/a      | ~329      | harmonic content (E4)    |
| `harmonics_1318`      | n/a      | ~1318     | harmonic content (E6)    |
| `Onset-Debug`         | n/a      | n/a       | onset debug, no test     |
| `Transient`           | n/a      | n/a       | transient debug, no test |
| `riff_0_0`            | n/a      | n/a       | debug, no test           |
| `riff_0_5`            | n/a      | n/a       | debug, no test           |
| `Pick-Scrape`         | n/a      | n/a       | pick scrape, unpitched   |
| `E-Chord`             | `low_e`  | 82.41     | strummed E chord, stereo |
| `E-Chord-Neo`         | `low_e`  | 82.41     | one held E chord, stereo |
| `E9-Chord`            | `low_e`  | 82.41     | one held E9 chord        |
| `D-Strings`           | n/a      | 146.83    | synthetic string pad     |
| `Piano-Chords`        | n/a      | 103.83    | piano, two chords        |

## Notes

- **`peaks.cpp` is the one exception to the open-string convention**: it bases
  samples on the *played pitch* (e.g. `1a-Low-E` → 329.64 Hz, High-E → 1318.52),
  because peak detection wants the sounding fundamental, not the string. Don't
  read that as a contradiction; every other detector test uses the open string
  above.
- `harmonics_261 / _329 / _1318` are named by their fundamental in Hz; use that
  as the base if you wire them in.
- The trailing `Onset-Debug`, `Transient`, `riff_0_*` are scratch/debug clips
  with no current test mapping; base frequency undetermined.
- `Pick-Scrape` is one continuous pick scrape along the strings, with
  nothing harmonic to lock to: for testing what a period or lag tracker
  does on noise. It is Azzaziel's "guitar pick scraping.wav" on Freesound
  (sound 347540, CC0), decoded from the site's 128 kbps MP3 preview to mono
  16-bit 44.1 kHz, so it is not a Nu Multi recording.
- `E-Chord` is a strummed open E chord, the stereo mix (24-bit) of the six
  hex tracks in `audio_tests/neo-e-chord`, outside the repo. Its common
  period, about 24.3 ms, is longer than any one string's. A mono test
  sums or picks a channel.
- `E-Chord-Neo` is one open E chord, struck once and left to ring out, for
  a whammy bar: no re-strum, a long smooth sustain. It is Neo take 19 of
  the "Simple Chords" session in `audio_tests` (outside the repo), from
  3.22 s to the end of the take (21.6 s), cut 0.25 s before the strum,
  faded in over 5 ms, otherwise as recorded, 24-bit. Both channels carry
  the whole chord, low E included, at different phase; a mono test sums
  them.
- `E9-Chord` is one clean E9 chord on a Stratocaster, struck once and left
  to ring (to 28 dB under its peak by the end), for a whammy bar. It is
  Mike Tribulas's "9577 Guitarz1970 Clean E9 Guitar Chord (Mike
  Tribulas).ogg" on Wikimedia Commons, licensed CC BY 2.5
  (https://creativecommons.org/licenses/by/2.5/). Modified: decoded from
  128 kbps Ogg Vorbis, the two channels (correlated at 0.95) averaged to
  mono, cut from 1.03 s (0.25 s before the strum) to the end (11.5 s),
  faded in over 5 ms, 24-bit.
- `D-Strings` is a synthetic string pad, a held D major chord (D3 F#3 A3
  D4 F#4 A4): each note three band-limited saws detuned -7, 0 and +6
  cents, each drifting +-2 cents at 0.2 Hz like an ensemble, harmonics
  to 9 kHz rolled off above 2 kHz; 0.25 s of silence, a 0.4 s bowed
  attack, a 1.5 s release at the end; 23 s, peak -12 dBFS, 24-bit. Its
  common period is 27 ms (D1), and its detuned voices drift apart within
  that period. Made for these tests, so no licence applies. The base is
  its lowest note, D3.
- `Piano-Chords` is a piano alternating two close-voiced chords over a
  G#2, struck seven times, about every 2 s; stretched partials and no
  clean common period. It is EKVelika's "Piano C Minor Major haunting
  chord" on Freesound (sound 168863, CC0), decoded from the site's
  128 kbps MP3 preview, left channel only (the channels partly cancel),
  17.6 s, 24-bit. The base is its lowest note, G#2.
