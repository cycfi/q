/*=============================================================================
   Copyright (C) 2012-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_MIDI_NOTE_HPP_SEPTEMBER_18_2026)
#define CYCFI_Q_MIDI_NOTE_HPP_SEPTEMBER_18_2026

#include <q/support/pitch_names.hpp>
#include <cstdint>
#include <string_view>

#if defined(B0)
# undef B0
#endif

namespace cycfi::q::midi
{
   ////////////////////////////////////////////////////////////////////////////
   // MIDI note to frequency
   ////////////////////////////////////////////////////////////////////////////
   constexpr frequency note_frequency(std::uint8_t key)
   {
      constexpr std::uint8_t lowest_key = 9;
      constexpr std::uint8_t highest_key = 119;

      if (key < lowest_key || key > highest_key)
         return frequency(0);
      auto octave = (key - lowest_key) / 12;
      auto semitone = (key - lowest_key) % 12;
      return pitch_frequencies[octave][semitone];
   }

   ////////////////////////////////////////////////////////////////////////////
   // MIDI note name
   ////////////////////////////////////////////////////////////////////////////
   constexpr char const* note_name(std::uint8_t key)
   {
      constexpr char const* name[] =
      {
         "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
         "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
         "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
         "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
         "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
         "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
         "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
         "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
         "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
         "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
         "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9"
      };

      return (key < 128)? name[key] : "--";
   }

   ////////////////////////////////////////////////////////////////////////////
   // MIDI note
   ////////////////////////////////////////////////////////////////////////////
   enum class note : std::uint8_t
   {
      C0 = 12
    , Cs0, Db0 = Cs0
    , D0
    , Ds0, Eb0 = Ds0
    , E0
    , F0
    , Fs0, Gb0 = Fs0
    , G0
    , Gs0, Ab0 = Gs0

    , A0 = 21
    , As0, Bb0 = As0
    , B0
    , C1
    , Cs1, Db1 = Cs1
    , D1
    , Ds1, Eb1 = Ds1
    , E1
    , F1
    , Fs1, Gb1 = Fs1
    , G1
    , Gs1, Ab1 = Gs1

    , A1 = 33
    , As1, Bb1 = As1
    , B1
    , C2
    , Cs2, Db2 = Cs2
    , D2
    , Ds2, Eb2 = Ds2
    , E2
    , F2
    , Fs2, Gb2 = Fs2
    , G2
    , Gs2, Ab2 = Gs2

    , A2 = 45
    , As2, Bb2 = As2
    , B2
    , C3
    , Cs3, Db3 = Cs3
    , D3
    , Ds3, Eb3 = Ds3
    , E3
    , F3
    , Fs3, Gb3 = Fs3
    , G3
    , Gs3, Ab3 = Gs3

    , A3 = 57
    , As3, Bb3 = As3
    , B3
    , C4
    , Cs4, Db4 = Cs4
    , D4
    , Ds4, Eb4 = Ds4
    , E4
    , F4
    , Fs4, Gb4 = Fs4
    , G4
    , Gs4, Ab4 = Gs4

    , A4 = 69
    , As4, Bb4 = As4
    , B4
    , C5
    , Cs5, Db5 = Cs5
    , D5
    , Ds5, Eb5 = Ds5
    , E5
    , F5
    , Fs5, Gb5 = Fs5
    , G5
    , Gs5, Ab5 = Gs5

    , A5 = 81
    , As5, Bb5 = As5
    , B5
    , C6
    , Cs6, Db6 = Cs6
    , D6
    , Ds6, Eb6 = Ds6
    , E6
    , F6
    , Fs6, Gb6 = Fs6
    , G6
    , Gs6, Ab6 = Gs6

    , A6 = 93
    , As6, Bb6 = As6
    , B6
    , C7
    , Cs7, Db7 = Cs7
    , D7
    , Ds7, Eb7 = Ds7
    , E7
    , F7
    , Fs7, Gb7 = Fs7
    , G7
    , Gs7, Ab7 = Gs7

    , A7 = 105
    , As7, Bb7 = As7
    , B7
    , C8
    , Cs8, Db8 = Cs8
    , D8
    , Ds8, Eb8 = Ds8
    , E8
    , F8
    , Fs8, Gb8 = Fs8
    , G8
    , Gs8, Ab8 = Gs8

    , A8 = 117
    , As8, Bb8 = As8
    , B8
    , C9
    , Cs9, Db9 = Cs9
    , D9
    , Ds9, Eb9 = Ds9
    , E9
    , F9
    , Fs9, Gb9 = Fs9
    , G9
   };

   ////////////////////////////////////////////////////////////////////////////
   // Key (e.g. "C5") to MIDI note number
   //
   // Returns the MIDI note number given the key (string): a letter, an
   // optional # or b, and the octave, -1 to 9. Returns -1 if parsing
   // failed or the key is not a MIDI note, 0 (C-1) to 127 (G9).
   ////////////////////////////////////////////////////////////////////////////
   inline int note_number(std::string_view note)
   {
      int n;
      auto iter = note.begin();
      if (iter != note.end())
      {
         // Get the base frequency
         switch (std::toupper(*iter++))
         {
            case 'A':   n = int(note::A0); break;
            case 'B':   n = int(note::B0); break;
            case 'C':   n = int(note::C0); break;
            case 'D':   n = int(note::D0); break;
            case 'E':   n = int(note::E0); break;
            case 'F':   n = int(note::F0); break;
            case 'G':   n = int(note::G0); break;
            default:    return -1;
         }

         if (iter != note.end())
         {
            // See if we want to shift up or down by a
            // semitone.
            if (*iter == '#')
            {
               ++n;
               ++iter;
            }
            else if (*iter == 'b')
            {
               --n;
               ++iter;
            }

            // The only signed octave is -1, the lowest.
            bool const negative = iter != note.end() && *iter == '-';
            if (negative)
               ++iter;

            if (iter != note.end() && std::isdigit(*iter))
            {
               // Set the octave
               int oct = *iter++ - '0';
               if (negative)
               {
                  if (oct != 1)
                     return -1;
                  oct = -1;
               }

               auto const key = n + oct * 12;
               if (iter == note.end() && key >= 0 && key <= 127)
                  return key;
            }
         }
      }
      return -1;
   }
}

namespace cycfi::q::midi_1_0
{
   using midi::note_frequency;
   using midi::note_name;
   using midi::note;
   using midi::note_number;
}

#endif
