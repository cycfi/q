# MPE conformance notes

Internal, like the dev log. What `q/midi/mpe.hpp` implements, clause by
clause, and where each rule is asserted. Not published (lives outside
`modules/`, so the Antora build ignores it).

## The source

MIDI Polyphonic Expression, version 1.0, March 12 2018. MMA/AMEI
Recommended Practice RP-053, published by the MIDI Manufacturers
Association and AMEI. Freely downloadable from midi.org; the copy read
for this work came from

    https://d30pueezughrda.cloudfront.net/campaigns/mpe/mpespec.pdf

The document is copyright MMA 2017, so it is not checked in here. Fetch
it when a rule needs re-reading. Tests quote only the fragment each case
turns on.

## Clause to test

Every case in `test/midi_mpe_spec.cpp` is named for its clause, so the
test output reads as a conformance report. In summary:

| Clause | Rule | Implemented as |
| --- | --- | --- |
| 2.1.1 | Configuration is registered parameter 00 06 | `parameter`, number 6 |
| 2.1.1 | Valid only on channel 1 or 16 | `configure` rejects the rest |
| 2.1.1 | Member count 0 turns the zone off, 1 to 15 sets it | count above 15 ignored |
| 2.1.1 | Lower zone counts up from channel 2, upper down from 15 | `zone_of` |
| 2.1.1 | An unused zone's master may be a member of the other | 15 members reach channel 16 |
| 2.1.1 | The newer message wins contested channels, even emptying a zone | `resolve_overlap` |
| 2.1.4 | Stop all notes and reset controls when a zone changes | `stop_all`, note offs emitted |
| 2.2.1 | A channel may hold several notes; channel messages reach them all | `channel_state::_keys` |
| 2.3.1 | Zone messages on a member channel are ignored | control change default case |
| 2.3.3 | Mode 3 ignores program change on a member channel | `operator()(program_change)` |
| 2.4 | Configuration sets ranges to 2 and 48 semitones | zone rebuilt on configure |
| 2.4 | Range 0 on the master is the zone's, on any member is all members' | `parameter`, number 0 |
| 2.4 | Master and member bend combine per sounding note | `send_pitch` |
| 2.5 | Master and member pressure combine per sounding note | `send_pressure` |
| 2.5 | Polyphonic key pressure not read on members, allowed on master | `operator()(poly_aftertouch)` |
| 2.6 | Master and member timbre combine per sounding note | `send_timbre` |
| 3.3 | Channel values are tracked with nothing sounding, and start the next note | `channel_state`, note on emits |
| 3.3 | A note stops following its channel once it has ended | key removed on note off |
| 3.3.5 | Timbre starts at 0x40 | `centre_timbre` |

## Where the specification leaves it to us

Three rules say "combine meaningfully" without saying how. What we chose,
and why:

- **Pitch** adds the two, each scaled by its own range, so a zone bend of
  two semitones under a note bent 24 reports 26. Any other reading breaks
  the tremolo-arm-plus-finger case the clause exists for.
- **Pressure** adds the two and clamps at 1. Both rest at zero, so a zone
  that sends none changes nothing.
- **Timbre** treats the zone's value as an offset from centre, because
  controller 74 rests at 0x40 rather than at zero. Adding outright would
  push every note bright the moment a zone sent its resting value.

Two more choices the specification does not constrain:

- A channel holds at most 8 notes. The specification requires sharing but
  sets no limit; 8 is well past what a controller does in practice, and
  the alternative is an allocation on the audio thread.
- Zone messages ignored on member channels are every control change other
  than 74 and the four that carry a registered parameter. Table 1 lists
  them individually; this is the same set by exclusion.

## Not covered

- MIDI Mode 4, where program change on a member channel is applied rather
  than ignored. Mode 3 is what MPE is designed for, and the mode messages
  that would switch it are not read yet.
- MIDI-CI Profile for MPE (2024), which negotiates MPE rather than
  declaring it. Relevant when MIDI 2.0 lands.
- Transmission. Everything here reads MPE; nothing sends it.
