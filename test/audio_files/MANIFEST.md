# Test audio — manifest

Base frequency (and identity) for every sample in this folder, so tests don't
have to rediscover it. **`base_freq` is the OPEN-STRING frequency** — the lowest
note the string can sound — which is what the pitch/period/onset detectors want
for sizing their period range (base_freq up to base_freq + range). It is NOT the
played pitch; a fretted note sits above the base.

Naming: `N{a,b,c}-<String>` — `a` = open, `b` = 12th fret (+1 octave played),
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
| `harmonics_261`       | —        | ~261      | harmonic content (C4)    |
| `harmonics_329`       | —        | ~329      | harmonic content (E4)    |
| `harmonics_1318`      | —        | ~1318     | harmonic content (E6)    |
| `Onset-Debug`         | —        | —         | onset debug, no test     |
| `Transient`           | —        | —         | transient debug, no test |
| `riff_0_0`            | —        | —         | debug, no test           |
| `riff_0_5`            | —        | —         | debug, no test           |

## Notes

- **`peaks.cpp` is the one exception to the open-string convention** — it bases
  samples on the *played pitch* (e.g. `1a-Low-E` → 329.64 Hz, High-E → 1318.52),
  because peak detection wants the sounding fundamental, not the string. Don't
  read that as a contradiction; every other detector test uses the open string
  above.
- `harmonics_261 / _329 / _1318` are named by their fundamental in Hz; use that
  as the base if you wire them in.
- The trailing `Onset-Debug`, `Transient`, `riff_0_*` are scratch/debug clips
  with no current test mapping — base frequency undetermined.
