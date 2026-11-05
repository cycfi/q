/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018)
#define CYCFI_Q_PITCH_DETECTOR_HPP_MARCH_12_2018

#include <q/pitch/bacf_pitch_detector.hpp>

namespace cycfi::q
{
   // Deprecated: `pitch_detector` was renamed to `bacf_pitch_detector`, after
   // its algorithm (Bitstream AutoCorrelation Function), as its period
   // detector is `bacf_period_detector`. This alias is kept for backward
   // compatibility and will be removed in a future release.
   using pitch_detector [[deprecated("renamed to bacf_pitch_detector")]]
      = bacf_pitch_detector;
}

#endif
