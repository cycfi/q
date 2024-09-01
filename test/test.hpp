/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_TEST_HPP_August_31_2024)
#define CYCFI_Q_TEST_HPP_August_31_2024

#include <q_io/audio_file.hpp>

namespace q = cycfi::q;

void compare_golden(char const* name, double tolerance = 1e-8)
{
   q::wav_reader src{ std::string("results/") + name + ".wav" };
   q::wav_reader golden{ std::string("golden/") + name + ".wav" };

   std::vector<float> a(src.length());
   src.read(a);

   std::vector<float> b(golden.length());
   golden.read(b);

   REQUIRE(a.size() == b.size());
   for (std::size_t i = 0; i < a.size(); ++i)
   {
      if (std::fabs(a[i] - b[i]) > tolerance)
      {
         FAIL(
            "In test: \"" << name << "\", at sample: " << i
            << ". Expecting: " << b[i] << ", got: " << a[i]
         );
      }
   }
}

#endif
