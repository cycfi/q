/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_DX7_ROUTING_HPP_SEPTEMBER_16_2026)
#define CYCFI_Q_DX7_ROUTING_HPP_SEPTEMBER_16_2026

#include <q/synth/fm/fm_routing.hpp>

namespace cycfi::q::dx7
{
   // Yamaha's numbering, for the chart below; the names stay here
   constexpr fm_op   op1 = fm_ops::op<1>, op2 = fm_ops::op<2>;
   constexpr fm_op   op3 = fm_ops::op<3>, op4 = fm_ops::op<4>;
   constexpr fm_op   op5 = fm_ops::op<5>, op6 = fm_ops::op<6>;

   ////////////////////////////////////////////////////////////////////////////
   // The DX7's 32 algorithms, its chart as routings: algorithm N is
   // dx7_routing[N - 1]. They are a good set of six operator topologies
   // whatever synth uses them, so they live here rather than with the
   // DX7's own parameters (see dx_patcher). An operator that only sounds
   // is named on its own, so every row says all six. A row is one line,
   // a chart being read a row at a time: the three longest run past the
   // usual limit rather than wrap.
   ////////////////////////////////////////////////////////////////////////////
   inline constexpr fm_routing dx7_routing[] =
   {
      // 1
      (op2 >> op1 | op6 >> op5 >> op4 >> op3).feedback(op6),
      // 2
      (op2 >> op1 | op6 >> op5 >> op4 >> op3).feedback(op2),
      // 3
      (op3 >> op2 >> op1 | op6 >> op5 >> op4).feedback(op6),
      // 4
      (op3 >> op2 >> op1 | op6 >> op5 >> op4).feedback(op4, op6),
      // 5
      (op2 >> op1 | op4 >> op3 | op6 >> op5).feedback(op6),
      // 6
      (op2 >> op1 | op4 >> op3 | op6 >> op5).feedback(op5, op6),
      // 7
      (op2 >> op1 | op4 >> op3 | op6 >> op5 >> op3).feedback(op6),
      // 8
      (op2 >> op1 | op4 >> op3 | op6 >> op5 >> op3).feedback(op4),
      // 9
      (op2 >> op1 | op4 >> op3 | op6 >> op5 >> op3).feedback(op2),
      // 10
      (op3 >> op2 >> op1 | op5 >> op4 | op6 >> op4).feedback(op3),
      // 11
      (op3 >> op2 >> op1 | op5 >> op4 | op6 >> op4).feedback(op6),
      // 12
      (op2 >> op1 | op4 >> op3 | op5 >> op3 | op6 >> op3).feedback(op2),
      // 13
      (op2 >> op1 | op4 >> op3 | op5 >> op3 | op6 >> op3).feedback(op6),
      // 14
      (op2 >> op1 | op4 >> op3 | op5 >> op4 | op6 >> op4).feedback(op6),
      // 15
      (op2 >> op1 | op4 >> op3 | op5 >> op4 | op6 >> op4).feedback(op2),
      // 16
      (op2 >> op1 | op3 >> op1 | op5 >> op1 | op4 >> op3 | op6 >> op5).feedback(op6),
      // 17
      (op2 >> op1 | op3 >> op1 | op5 >> op1 | op4 >> op3 | op6 >> op5).feedback(op2),
      // 18
      (op2 >> op1 | op3 >> op1 | op4 >> op1 | op6 >> op5 >> op4).feedback(op3),
      // 19
      (op3 >> op2 >> op1 | op6 >> op4 | op6 >> op5).feedback(op6),
      // 20
      (op3 >> op1 | op3 >> op2 | op5 >> op4 | op6 >> op4).feedback(op3),
      // 21
      (op3 >> op1 | op3 >> op2 | op6 >> op4 | op6 >> op5).feedback(op3),
      // 22
      (op2 >> op1 | op6 >> op3 | op6 >> op4 | op6 >> op5).feedback(op6),
      // 23
      (op3 >> op2 | op6 >> op4 | op6 >> op5 | op1).feedback(op6),
      // 24
      (op6 >> op3 | op6 >> op4 | op6 >> op5 | op1 | op2).feedback(op6),
      // 25
      (op6 >> op4 | op6 >> op5 | op1 | op2 | op3).feedback(op6),
      // 26
      (op3 >> op2 | op5 >> op4 | op6 >> op4 | op1).feedback(op6),
      // 27
      (op3 >> op2 | op5 >> op4 | op6 >> op4 | op1).feedback(op3),
      // 28
      (op2 >> op1 | op5 >> op4 >> op3 | op6).feedback(op5),
      // 29
      (op4 >> op3 | op6 >> op5 | op1 | op2).feedback(op6),
      // 30
      (op5 >> op4 >> op3 | op1 | op2 | op6).feedback(op5),
      // 31
      (op6 >> op5 | op1 | op2 | op3 | op4).feedback(op6),
      // 32
      (op1 | op2 | op3 | op4 | op5 | op6).feedback(op6)
   };
}

namespace cycfi::q
{
   using dx7::dx7_routing;
}

#endif
