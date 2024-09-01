/*=============================================================================
   Copyright (C) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_NAMES_HPP_APRIL_20_2018)
#define CYCFI_Q_PITCH_NAMES_HPP_APRIL_20_2018

#include <q/support/literals.hpp>

namespace cycfi::q
{
   // We need this because we don't have A constexpr std::pow
   constexpr auto _12th_root = 1.059463094359295;

   constexpr frequency next_frequency(frequency F)
   {
      return F * _12th_root;
   }

   struct octave_pitches
   {
      // Oddity: C is actually the start of an octave, not A. So, an octave
      // starts from C to B, not A to G. For example C0 to B0 is the first
      // (lowest) octave. So that is why we have: C = next_frequency(B) / 2
      // in the ctor below.

      constexpr octave_pitches(frequency base)
       : A     (base)
       , As    (next_frequency(A))
       , B     (next_frequency(As))
       , C     (next_frequency(B) / 2)
       , Cs    (next_frequency(C))
       , D     (next_frequency(Cs))
       , Ds    (next_frequency(D))
       , E     (next_frequency(Ds))
       , F     (next_frequency(E))
       , Fs    (next_frequency(F))
       , G     (next_frequency(Fs))
       , Gs    (next_frequency(G))

       // Aliases
       , Ab  (Gs)
       , Bb  (As)
       , Db  (Cs)
       , eb  (Ds)
       , Gb  (Fs)
      {}

      frequency A, As, B, C, Cs, D, Ds, E, F, Fs, G, Gs;
      frequency Ab, Bb, Db, eb, Gb;
   };

#if defined(_MSC_VER) && (_MSC_VER <= 1910)
# define CONSTEXPR const
#else
# define CONSTEXPR constexpr
#endif

   CONSTEXPR octave_pitches oct_pitch[] =
   {
      frequency(27.5)
    , frequency(55)
    , frequency(110)
    , frequency(220)
    , frequency(440)
    , frequency(880)
    , frequency(1760)
    , frequency(3520)
    , frequency(7040)
   };

   struct octave_frequencies
   {
      CONSTEXPR octave_frequencies(frequency base)
       : f
       { base
       , next_frequency(f[0])
       , next_frequency(f[1])
       , next_frequency(f[2])
       , next_frequency(f[3])
       , next_frequency(f[4])
       , next_frequency(f[5])
       , next_frequency(f[6])
       , next_frequency(f[7])
       , next_frequency(f[8])
       , next_frequency(f[9])
       , next_frequency(f[10])
       }
      {}

      CONSTEXPR frequency operator[](std::size_t semitone) const
      {
         return f[semitone];
      }

      frequency f[12];
   };

   CONSTEXPR octave_frequencies pitch_frequencies[] =
   {
      { frequency(13.75) }
    , { frequency(27.5) }
    , { frequency(55) }
    , { frequency(110) }
    , { frequency(220) }
    , { frequency(440) }
    , { frequency(880) }
    , { frequency(1760) }
    , { frequency(3520) }
    , { frequency(7040) }
   };

   namespace pitch_names
   {
      CONSTEXPR frequency Ab[] =
      {
         oct_pitch[0].Ab
       , oct_pitch[1].Ab
       , oct_pitch[2].Ab
       , oct_pitch[3].Ab
       , oct_pitch[4].Ab
       , oct_pitch[5].Ab
       , oct_pitch[6].Ab
       , oct_pitch[7].Ab
      };

      CONSTEXPR frequency A[] =
      {
         oct_pitch[0].A
       , oct_pitch[1].A
       , oct_pitch[2].A
       , oct_pitch[3].A
       , oct_pitch[4].A
       , oct_pitch[5].A
       , oct_pitch[6].A
       , oct_pitch[7].A
      };

      CONSTEXPR frequency As[] =
      {
         oct_pitch[0].As
       , oct_pitch[1].As
       , oct_pitch[2].As
       , oct_pitch[3].As
       , oct_pitch[4].As
       , oct_pitch[5].As
       , oct_pitch[6].As
       , oct_pitch[7].As
      };

      CONSTEXPR frequency Bb[] =
      {
         oct_pitch[0].Bb
       , oct_pitch[1].Bb
       , oct_pitch[2].Bb
       , oct_pitch[3].Bb
       , oct_pitch[4].Bb
       , oct_pitch[5].Bb
       , oct_pitch[6].Bb
       , oct_pitch[7].Bb
      };

      CONSTEXPR frequency B[] =
      {
         oct_pitch[0].B
       , oct_pitch[1].B
       , oct_pitch[2].B
       , oct_pitch[3].B
       , oct_pitch[4].B
       , oct_pitch[5].B
       , oct_pitch[6].B
       , oct_pitch[7].B
      };

      CONSTEXPR frequency C[] =
      {
         oct_pitch[0].C
       , oct_pitch[1].C
       , oct_pitch[2].C
       , oct_pitch[3].C
       , oct_pitch[4].C
       , oct_pitch[5].C
       , oct_pitch[6].C
       , oct_pitch[7].C
      };

      CONSTEXPR frequency Cs[] =
      {
         oct_pitch[0].Cs
       , oct_pitch[1].Cs
       , oct_pitch[2].Cs
       , oct_pitch[3].Cs
       , oct_pitch[4].Cs
       , oct_pitch[5].Cs
       , oct_pitch[6].Cs
       , oct_pitch[7].Cs
      };

      CONSTEXPR frequency Db[] =
      {
         oct_pitch[0].Db
       , oct_pitch[1].Db
       , oct_pitch[2].Db
       , oct_pitch[3].Db
       , oct_pitch[4].Db
       , oct_pitch[5].Db
       , oct_pitch[6].Db
       , oct_pitch[7].Db
      };

      CONSTEXPR frequency D[] =
      {
         oct_pitch[0].D
       , oct_pitch[1].D
       , oct_pitch[2].D
       , oct_pitch[3].D
       , oct_pitch[4].D
       , oct_pitch[5].D
       , oct_pitch[6].D
       , oct_pitch[7].D
      };

      CONSTEXPR frequency Ds[] =
      {
         oct_pitch[0].Ds
       , oct_pitch[1].Ds
       , oct_pitch[2].Ds
       , oct_pitch[3].Ds
       , oct_pitch[4].Ds
       , oct_pitch[5].Ds
       , oct_pitch[6].Ds
       , oct_pitch[7].Ds
      };

      CONSTEXPR frequency Eb[] =
      {
         oct_pitch[0].E
       , oct_pitch[1].E
       , oct_pitch[2].E
       , oct_pitch[3].E
       , oct_pitch[4].E
       , oct_pitch[5].E
       , oct_pitch[6].E
       , oct_pitch[7].E
      };

      CONSTEXPR frequency E[] =
      {
         oct_pitch[0].E
       , oct_pitch[1].E
       , oct_pitch[2].E
       , oct_pitch[3].E
       , oct_pitch[4].E
       , oct_pitch[5].E
       , oct_pitch[6].E
       , oct_pitch[7].E
      };

      CONSTEXPR frequency F[] =
      {
         oct_pitch[0].F
       , oct_pitch[1].F
       , oct_pitch[2].F
       , oct_pitch[3].F
       , oct_pitch[4].F
       , oct_pitch[5].F
       , oct_pitch[6].F
       , oct_pitch[7].F
      };

      CONSTEXPR frequency Fs[] =
      {
         oct_pitch[0].Fs
       , oct_pitch[1].Fs
       , oct_pitch[2].Fs
       , oct_pitch[3].Fs
       , oct_pitch[4].Fs
       , oct_pitch[5].Fs
       , oct_pitch[6].Fs
       , oct_pitch[7].Fs
      };

      CONSTEXPR frequency Gb[] =
      {
         oct_pitch[0].Gb
       , oct_pitch[1].Gb
       , oct_pitch[2].Gb
       , oct_pitch[3].Gb
       , oct_pitch[4].Gb
       , oct_pitch[5].Gb
       , oct_pitch[6].Gb
       , oct_pitch[7].Gb
      };

      CONSTEXPR frequency G[] =
      {
         oct_pitch[0].G
       , oct_pitch[1].G
       , oct_pitch[2].G
       , oct_pitch[3].G
       , oct_pitch[4].G
       , oct_pitch[5].G
       , oct_pitch[6].G
       , oct_pitch[7].G
      };

      CONSTEXPR frequency Gs[] =
      {
         oct_pitch[0].Gs
       , oct_pitch[1].Gs
       , oct_pitch[2].Gs
       , oct_pitch[3].Gs
       , oct_pitch[4].Gs
       , oct_pitch[5].Gs
       , oct_pitch[6].Gs
       , oct_pitch[7].Gs
      };
   }
}

#endif

