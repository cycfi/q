/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_EDGE_DECEMBER_7_2018)
#define CYCFI_Q_EDGE_DECEMBER_7_2018

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // rising_edge detects rising edges (i.e returns 1 when the input
   // transitions from 0 to 1).
   ////////////////////////////////////////////////////////////////////////////
   struct rising_edge
   {
      constexpr bool operator()(bool val)
      {
         auto r = val && (_state != val);
         _state = val;
         return r;
      }
      bool _state = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // falling_edge detects falling edges (i.e returns 1 when the input
   // transitions from 1 to 0).
   ////////////////////////////////////////////////////////////////////////////
   struct falling_edge
   {
      constexpr bool operator()(bool val)
      {
         auto r = !val && (_state != val);
         _state = val;
         return r;
      }
      bool _state = 0;
   };
}

#endif
