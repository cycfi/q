/*=============================================================================
   Copyright (C) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_NOTES_HPP_APRIL_20_2018)
#define CYCFI_Q_NOTES_HPP_APRIL_20_2018

#include <q/support/literals.hpp>

namespace cycfi::q
{
   // We need this because we don't have A constexpr std::pow
   constexpr auto _12th_root = 1.059463094359295;

   constexpr frequency next_frequency(frequency F)
   {
      return F * _12th_root;
   }

   struct octave_notes
   {
      constexpr octave_notes(frequency base)
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

   CONSTEXPR octave_notes note[] =
   {
      { 27.5 }
    , { 55 }
    , { 110 }
    , { 220 }
    , { 440 }
    , { 880 }
    , { 1760 }
    , { 3520 }
    , { 7040 }
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

   CONSTEXPR octave_frequencies note_frequencies[] =
   {
      { 13.75 }
    , { 27.5 }
    , { 55 }
    , { 110 }
    , { 220 }
    , { 440 }
    , { 880 }
    , { 1760 }
    , { 3520 }
    , { 7040 }
   };

   namespace notes
   {
      CONSTEXPR frequency Ab[] =
      {
         note[0].Ab
       , note[1].Ab
       , note[2].Ab
       , note[3].Ab
       , note[4].Ab
       , note[5].Ab
       , note[6].Ab
       , note[7].Ab
      };

      CONSTEXPR frequency A[] =
      {
         note[0].A
       , note[1].A
       , note[2].A
       , note[3].A
       , note[4].A
       , note[5].A
       , note[6].A
       , note[7].A
      };

      CONSTEXPR frequency As[] =
      {
         note[0].As
       , note[1].As
       , note[2].As
       , note[3].As
       , note[4].As
       , note[5].As
       , note[6].As
       , note[7].As
      };

      CONSTEXPR frequency Bb[] =
      {
         note[0].Bb
       , note[1].Bb
       , note[2].Bb
       , note[3].Bb
       , note[4].Bb
       , note[5].Bb
       , note[6].Bb
       , note[7].Bb
      };

      CONSTEXPR frequency B[] =
      {
         note[0].B
       , note[1].B
       , note[2].B
       , note[3].B
       , note[4].B
       , note[5].B
       , note[6].B
       , note[7].B
      };

      CONSTEXPR frequency C[] =
      {
         note[0].C
       , note[1].C
       , note[2].C
       , note[3].C
       , note[4].C
       , note[5].C
       , note[6].C
       , note[7].C
      };

      CONSTEXPR frequency Cs[] =
      {
         note[0].Cs
       , note[1].Cs
       , note[2].Cs
       , note[3].Cs
       , note[4].Cs
       , note[5].Cs
       , note[6].Cs
       , note[7].Cs
      };

      CONSTEXPR frequency Db[] =
      {
         note[0].Db
       , note[1].Db
       , note[2].Db
       , note[3].Db
       , note[4].Db
       , note[5].Db
       , note[6].Db
       , note[7].Db
      };

      CONSTEXPR frequency D[] =
      {
         note[0].D
       , note[1].D
       , note[2].D
       , note[3].D
       , note[4].D
       , note[5].D
       , note[6].D
       , note[7].D
      };

      CONSTEXPR frequency Ds[] =
      {
         note[0].Ds
       , note[1].Ds
       , note[2].Ds
       , note[3].Ds
       , note[4].Ds
       , note[5].Ds
       , note[6].Ds
       , note[7].Ds
      };

      CONSTEXPR frequency eb[] =
      {
         note[0].E
       , note[1].E
       , note[2].E
       , note[3].E
       , note[4].E
       , note[5].E
       , note[6].E
       , note[7].E
      };

      CONSTEXPR frequency E[] =
      {
         note[0].E
       , note[1].E
       , note[2].E
       , note[3].E
       , note[4].E
       , note[5].E
       , note[6].E
       , note[7].E
      };

      CONSTEXPR frequency F[] =
      {
         note[0].F
       , note[1].F
       , note[2].F
       , note[3].F
       , note[4].F
       , note[5].F
       , note[6].F
       , note[7].F
      };

      CONSTEXPR frequency Fs[] =
      {
         note[0].Fs
       , note[1].Fs
       , note[2].Fs
       , note[3].Fs
       , note[4].Fs
       , note[5].Fs
       , note[6].Fs
       , note[7].Fs
      };

      CONSTEXPR frequency Gb[] =
      {
         note[0].Gb
       , note[1].Gb
       , note[2].Gb
       , note[3].Gb
       , note[4].Gb
       , note[5].Gb
       , note[6].Gb
       , note[7].Gb
      };

      CONSTEXPR frequency G[] =
      {
         note[0].G
       , note[1].G
       , note[2].G
       , note[3].G
       , note[4].G
       , note[5].G
       , note[6].G
       , note[7].G
      };

      CONSTEXPR frequency Gs[] =
      {
         note[0].Gs
       , note[1].Gs
       , note[2].Gs
       , note[3].Gs
       , note[4].Gs
       , note[5].Gs
       , note[6].Gs
       , note[7].Gs
      };
   }
}

#endif

