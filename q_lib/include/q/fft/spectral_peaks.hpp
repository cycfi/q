/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_SPECTRAL_PEAKS_OCTOBER_31_2026)
#define CYCFI_Q_SPECTRAL_PEAKS_OCTOBER_31_2026

#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // spectral_peaks: the peaks of the n/2+1 bin magnitude spectrum of an
   // n-point transform, and the bins each peak owns. A peak is a bin above
   // `threshold` and above its two neighbours on either side (Laroche and
   // Dolson's rule). Between two adjacent peaks, the lowest bin is the
   // boundary: it and everything below it belong to the lower peak. The
   // first peak also owns the bins down to 0, and the last up to n/2, so
   // the regions tile the spectrum.
   //
   // Each call finds the peaks of one spectrum. Room for every possible
   // peak is made at construction, so a call never allocates.
   ////////////////////////////////////////////////////////////////////////////
   class spectral_peaks
   {
   public:

      struct peak
      {
         std::size_t       bin;     // the peak
         std::size_t       first;   // the first bin it owns
         std::size_t       last;    // the last bin it owns
      };

      using peak_span = std::span<peak const>;

      explicit             spectral_peaks(
                              std::size_t n
                            , float threshold = 0.0f
                           );

      void                 operator()(std::span<float const> mag);

      peak_span            peaks() const;
      std::size_t          size() const;
      float                threshold() const;
      void                 threshold(float t);

   private:

      std::size_t          _n;
      float                _threshold;
      std::size_t          _count = 0;
      std::vector<peak>    _peaks;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   inline spectral_peaks::spectral_peaks(std::size_t n, float threshold)
    : _n{n}
    , _threshold{threshold}
    , _peaks(n/4 + 1)
   {}

   inline void spectral_peaks::operator()(std::span<float const> mag)
   {
      auto const last_bin = _n/2;
      assert(mag.size() >= last_bin + 1);

      auto above = [&](std::size_t k, std::size_t j)
      {
         return j > last_bin || mag[k] > mag[j];
      };

      _count = 0;
      for (std::size_t k = 0; k <= last_bin; ++k)
      {
         bool is_peak = mag[k] > _threshold
            && (k < 1 || mag[k] > mag[k-1])
            && (k < 2 || mag[k] > mag[k-2])
            && above(k, k+1) && above(k, k+2);
         if (is_peak)
            _peaks[_count++] = {k, 0, last_bin};
      }

      // Split each pair of neighbours at the lowest bin between them.
      for (std::size_t i = 0; i + 1 < _count; ++i)
      {
         auto valley = _peaks[i].bin + 1;
         for (auto k = valley; k < _peaks[i+1].bin; ++k)
         {
            if (mag[k] < mag[valley])
               valley = k;
         }
         _peaks[i].last = valley;
         _peaks[i+1].first = valley + 1;
      }
   }

   inline spectral_peaks::peak_span spectral_peaks::peaks() const
   {
      return {_peaks.data(), _count};
   }

   inline std::size_t spectral_peaks::size() const
   {
      return _n;
   }

   inline float spectral_peaks::threshold() const
   {
      return _threshold;
   }

   inline void spectral_peaks::threshold(float t)
   {
      _threshold = t;
   }
}

#endif
