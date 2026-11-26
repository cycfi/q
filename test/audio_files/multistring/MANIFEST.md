# Multistring test audio manifest

Multistring recordings: one string per channel, channel 1 the highest
string, so a guitar with more strings adds its extra bass strings as
channels 7, 8 and on. The audio is not in git: it is the R2 test data set
`multistring` (`test/testdata/sets.txt`), fetched into the test data cache
when a test that needs it runs (`q_testdata(<test> multistring)` in CMake,
`testdata.path("multistring")` in Python). This manifest is the record of
what is in the set.

`base` is each channel's OPEN-STRING frequency, as in `../MANIFEST.md`:
the lowest note the string can sound, which sizes a detector's period
range. It is not the played pitch.

## Samples

### `nu_v2_mittino_nst.flac`

A Nu V2 installed in Fabio Mittino's guitar, in NST tuning (C2 G2 D3 A3
E4 G4). 48 kHz, 16-bit, 60 s, six channels. The raw pickup signal of each
string, no gain: peaks -5.7 to -12.7 dBFS, noise floor about -76 dBFS.
The crosstalk between strings is left in, so the file also tests bleed
handling: a picked string reaches its neighbours as an inverted copy at
about -32 dB.

| channel | string | note | base (Hz) | peak (dBFS) |
|---------|--------|------|-----------|-------------|
| 1       | 1      | G4   | 392.00    | -10.5       |
| 2       | 2      | E4   | 329.63    | -12.7       |
| 3       | 3      | A3   | 220.00    | -11.4       |
| 4       | 4      | D3   | 146.83    | -7.9        |
| 5       | 5      | G2   | 98.00     | -5.7        |
| 6       | 6      | C2   | 65.41     | -6.6        |

Made from six stereo WAVs, one per string (left and right the same to
-56 dBFS): each channel is its file's left channel, bit for bit. The file
carries the tuning in its own tags (`TUNING`, `CHANNELS`).
