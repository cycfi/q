/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
// The native half of fft_bench: the same frames without Q_USE_PFFFT.
#undef Q_USE_PFFFT
#include "fft_bench_common.hpp"

namespace fft_bench::native
{
   frame_times times(std::size_t n)
   {
      return measure(n);
   }
}
