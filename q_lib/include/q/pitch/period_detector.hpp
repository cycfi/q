/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0.
   [ https://www.boost.org/LICENSE_1_0.txt ]
=============================================================================*/
#if !defined(CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PERIOD_DETECTOR_HPP_MARCH_12_2018

#include <q/pitch/bacf_period_detector.hpp>

namespace cycfi::q
{
   // Deprecated: `period_detector` was renamed to `bacf_period_detector` to
   // name it after its algorithm (Bitstream AutoCorrelation Function) and to
   // disambiguate from hz's correlator-based detector. This alias is kept for
   // backward compatibility and will be removed in a future release.
   using period_detector [[deprecated("renamed to bacf_period_detector")]]
      = bacf_period_detector;
}

#endif
