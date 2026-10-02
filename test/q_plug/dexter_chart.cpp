/*=============================================================================
   Copyright (c) 2019-2026 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The editor's algorithm pictures are drawn by gen_algorithms.py from Q's
// dx7_routing, and the table of where their boxes are comes with them. Each
// of the 32 is checked for what a reader of the chart relies on: every box
// in its cell, no two overlapping, the carriers along the bottom, and every
// modulator above what it modulates.
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/synth/fm/dx7_routing.hpp>
#include "algorithm_boxes.hpp"

#include <algorithm>

namespace q = cycfi::q;

namespace
{
   struct box
   {
      float left, top, right, bottom;
   };

   box box_of(int algorithm, int op)
   {
      auto const& b = algorithm_boxes[algorithm][op];
      return {b[0], b[1], b[2], b[3]};
   }

   bool overlap(box a, box b)
   {
      return a.left < b.right && b.left < a.right
         && a.top < b.bottom && b.top < a.bottom;
   }
}

TEST_CASE("Every algorithm's chart is readable")
{
   for (int n = 0; n != 32; ++n)
   {
      INFO("algorithm " << n + 1);
      auto const t = q::dx7_routing[n].get();

      float bottom = 0;
      for (int i = 0; i != 6; ++i)
         bottom = std::max(bottom, box_of(n, i).bottom);

      for (int i = 0; i != 6; ++i)
      {
         INFO("operator " << i + 1);
         auto const b = box_of(n, i);
         CHECK(b.left >= 0.0f);
         CHECK(b.top >= 0.0f);
         CHECK(b.right <= 1.0f);
         CHECK(b.bottom <= 1.0f);

         auto const carrier = (t.carriers & (1u << i)) != 0;
         CHECK(carrier == (b.bottom == Approx(bottom)));

         for (int m = 0; m != 6; ++m)
         {
            if (m != i)
               CHECK_FALSE(overlap(b, box_of(n, m)));
            if (t.mod[i] & (1u << m))
               CHECK(box_of(n, m).bottom < b.top);
         }
      }
   }
}
