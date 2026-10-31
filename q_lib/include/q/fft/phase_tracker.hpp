/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_PHASE_TRACKER_OCTOBER_31_2026)
#define CYCFI_Q_PHASE_TRACKER_OCTOBER_31_2026

#include <q/support/base.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // phase_tracker: the instantaneous frequency of each of the n/2+1 bins of
   // an n-point transform taken every `hop` samples, from the phases of
   // consecutive frames. At its centre frequency, bin k's phase advances by
   // 2 pi k hop / n a hop; the rest of the advance, wrapped to [-pi, pi], is
   // the offset from the centre. The result is in bins, k plus a fraction:
   // times sps / n, it is in Hz.
   //
   // The wrap makes an offset of more than n / (2 hop) bins ambiguous: 2
   // bins either way at a hop of n/4. The first frame after construction or
   // reset has no previous phase, and gives each bin its centre.
   ////////////////////////////////////////////////////////////////////////////
   class phase_tracker
   {
   public:

                           phase_tracker(std::size_t n, std::size_t hop);

      void                 operator()(
                              std::span<float const> phase
                            , std::span<float> freq
                           );

      std::size_t          size() const;
      std::size_t          hop() const;
      void                 reset();

   private:

      std::size_t          _n;
      std::size_t          _hop;
      bool                 _primed = false;
      std::vector<float>   _prev;
      std::vector<float>   _advance;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline phase_tracker::phase_tracker(std::size_t n, std::size_t hop)
    : _n{n}
    , _hop{hop}
    , _prev(n/2 + 1)
    , _advance(n/2 + 1)
   {
      // Each bin's advance a hop at its centre, wrapped, computed in double
      // so a large k * hop loses nothing.
      for (std::size_t k = 0; k <= n/2; ++k)
      {
         double a = 2 * pi * double(k) * double(hop) / n;
         _advance[k] = float(a - 2 * pi * std::floor(a / (2 * pi) + 0.5));
      }
   }

   inline void phase_tracker::operator()(
      std::span<float const> phase
    , std::span<float> freq)
   {
      assert(phase.size() >= _n/2 + 1 && freq.size() >= _n/2 + 1);

      float const to_bins = float(_n / (2 * pi * double(_hop)));
      for (std::size_t k = 0; k <= _n/2; ++k)
      {
         float offset = _primed?
            principal_arg(phase[k] - _prev[k] - _advance[k]) : 0.0f;
         freq[k] = float(k) + offset * to_bins;
         _prev[k] = phase[k];
      }
      _primed = true;
   }

   inline std::size_t phase_tracker::size() const
   {
      return _n;
   }

   inline std::size_t phase_tracker::hop() const
   {
      return _hop;
   }

   inline void phase_tracker::reset()
   {
      _primed = false;
      std::fill(_prev.begin(), _prev.end(), 0.0f);
   }
}

#endif
