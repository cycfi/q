/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_NOTES_HPP_APRIL_20_2018)
#define CYCFI_Q_NOTES_HPP_APRIL_20_2018

#include <q/literals.hpp>

namespace cycfi { namespace q
{
   // We need this because we don't have a constexpr std::pow
   constexpr auto _12th_root = 1.059463094359295;

   constexpr frequency next_frequency(frequency f)
   {
      return f * _12th_root;
   }

   struct octave_notes
   {
      constexpr octave_notes(frequency base)
       : a     (base)
       , as    (next_frequency(a))
       , b     (next_frequency(as))
       , c     (next_frequency(b) / 2)
       , cs    (next_frequency(c))
       , d     (next_frequency(cs))
       , ds    (next_frequency(d))
       , e     (next_frequency(ds))
       , f     (next_frequency(e))
       , fs    (next_frequency(f))
       , g     (next_frequency(fs))
       , gs    (next_frequency(g))

       // Aliases
       , ab  (gs)
       , bb  (as)
       , db  (cs)
       , eb  (ds)
       , gb  (fs)
      {}

      frequency a, as, b, c, cs, d, ds, e, f, fs, g, gs;
      frequency ab, bb, db, eb, gb;
   };

   constexpr octave_notes note[] =
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

   namespace notes
   {
      constexpr frequency ab[] =
      {
         note[0].ab
       , note[1].ab
       , note[2].ab
       , note[3].ab
       , note[4].ab
       , note[5].ab
       , note[6].ab
       , note[7].ab
      };

      constexpr frequency a[] =
      {
         note[0].a
       , note[1].a
       , note[2].a
       , note[3].a
       , note[4].a
       , note[5].a
       , note[6].a
       , note[7].a
      };

      constexpr frequency as[] =
      {
         note[0].as
       , note[1].as
       , note[2].as
       , note[3].as
       , note[4].as
       , note[5].as
       , note[6].as
       , note[7].as
      };

      constexpr frequency bb[] =
      {
         note[0].bb
       , note[1].bb
       , note[2].bb
       , note[3].bb
       , note[4].bb
       , note[5].bb
       , note[6].bb
       , note[7].bb
      };

      constexpr frequency b[] =
      {
         note[0].b
       , note[1].b
       , note[2].b
       , note[3].b
       , note[4].b
       , note[5].b
       , note[6].b
       , note[7].b
      };

      constexpr frequency c[] =
      {
         note[0].c
       , note[1].c
       , note[2].c
       , note[3].c
       , note[4].c
       , note[5].c
       , note[6].c
       , note[7].c
      };

      constexpr frequency cs[] =
      {
         note[0].cs
       , note[1].cs
       , note[2].cs
       , note[3].cs
       , note[4].cs
       , note[5].cs
       , note[6].cs
       , note[7].cs
      };

      constexpr frequency db[] =
      {
         note[0].db
       , note[1].db
       , note[2].db
       , note[3].db
       , note[4].db
       , note[5].db
       , note[6].db
       , note[7].db
      };

      constexpr frequency d[] =
      {
         note[0].d
       , note[1].d
       , note[2].d
       , note[3].d
       , note[4].d
       , note[5].d
       , note[6].d
       , note[7].d
      };

      constexpr frequency ds[] =
      {
         note[0].ds
       , note[1].ds
       , note[2].ds
       , note[3].ds
       , note[4].ds
       , note[5].ds
       , note[6].ds
       , note[7].ds
      };

      constexpr frequency eb[] =
      {
         note[0].e
       , note[1].e
       , note[2].e
       , note[3].e
       , note[4].e
       , note[5].e
       , note[6].e
       , note[7].e
      };

      constexpr frequency e[] =
      {
         note[0].e
       , note[1].e
       , note[2].e
       , note[3].e
       , note[4].e
       , note[5].e
       , note[6].e
       , note[7].e
      };

      constexpr frequency f[] =
      {
         note[0].f
       , note[1].f
       , note[2].f
       , note[3].f
       , note[4].f
       , note[5].f
       , note[6].f
       , note[7].f
      };

      constexpr frequency fs[] =
      {
         note[0].fs
       , note[1].fs
       , note[2].fs
       , note[3].fs
       , note[4].fs
       , note[5].fs
       , note[6].fs
       , note[7].fs
      };

      constexpr frequency gb[] =
      {
         note[0].gb
       , note[1].gb
       , note[2].gb
       , note[3].gb
       , note[4].gb
       , note[5].gb
       , note[6].gb
       , note[7].gb
      };

      constexpr frequency g[] =
      {
         note[0].g
       , note[1].g
       , note[2].g
       , note[3].g
       , note[4].g
       , note[5].g
       , note[6].g
       , note[7].g
      };

      constexpr frequency gs[] =
      {
         note[0].gs
       , note[1].gs
       , note[2].gs
       , note[3].gs
       , note[4].gs
       , note[5].gs
       , note[6].gs
       , note[7].gs
      };
   }
}}

#endif

