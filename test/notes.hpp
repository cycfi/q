/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <q/support/notes.hpp>

namespace notes
{
   using namespace cycfi::q::notes;
   using frequency = cycfi::q::frequency;

   CONSTEXPR frequency low_fs         = Fs[1];
   CONSTEXPR frequency low_b          = B[1];
   CONSTEXPR frequency low_e          = E[2];
   CONSTEXPR frequency a              = A[2];
   CONSTEXPR frequency d              = D[3];
   CONSTEXPR frequency g              = G[3];
   CONSTEXPR frequency b              = B[3];
   CONSTEXPR frequency high_e         = E[4];

   CONSTEXPR frequency low_e_12th     = E[3];
   CONSTEXPR frequency a_12th         = A[3];
   CONSTEXPR frequency d_12th         = D[4];
   CONSTEXPR frequency g_12th         = G[4];
   CONSTEXPR frequency b_12th         = B[4];
   CONSTEXPR frequency high_e_12th    = E[5];

   CONSTEXPR frequency low_e_24th     = E[4];
   CONSTEXPR frequency a_24th         = A[4];
   CONSTEXPR frequency d_24th         = D[5];
   CONSTEXPR frequency g_24th         = G[5];
   CONSTEXPR frequency b_24th         = B[5];
   CONSTEXPR frequency high_e_24th    = E[6];

   CONSTEXPR frequency middle_c       = C[4];
}

