/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_STFT_WINDOW_OCTOBER_31_2026)
#define CYCFI_Q_STFT_WINDOW_OCTOBER_31_2026

#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // stft_window: the analysis and synthesis windows of a short-time
   // Fourier transform of n samples. Overlap-adding analysis * synthesis at
   // the hop gives exactly 1 at every sample: the overlap-add gain is folded
   // into the synthesis window, so a frame that is left alone comes back
   // unchanged, `latency` samples later.
   //
   // hann: Hann analysis and synthesis over the whole frame. The latency is
   // n.
   //
   // low_delay: an asymmetric pair. The analysis window spans the whole
   // frame, for the frequency resolution of n samples, while the product of
   // the two is a Hann of `delay` samples at the newest end of the frame, so
   // the latency is `delay`. After Mauler and Martin, "A low delay, variable
   // resolution, perfect reconstruction spectral analysis-synthesis system
   // for speech enhancement", EUSIPCO 2007.
   ////////////////////////////////////////////////////////////////////////////
   struct stft_window
   {
      static stft_window   hann(std::size_t n, std::size_t hop);
      static stft_window   low_delay(
                              std::size_t n
                            , std::size_t hop
                            , std::size_t delay
                           );

      std::size_t          size() const;

      std::vector<float>   analysis;
      std::vector<float>   synthesis;
      std::size_t          latency = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   namespace detail
   {
      // Periodic Hann of length L at n, 0 <= n < L.
      inline double periodic_hann(std::size_t n, std::size_t L)
      {
         return 0.5 - 0.5 * std::cos(2 * std::numbers::pi * n / L);
      }

      // Divide the synthesis window, point by point, by the overlap-add sum
      // of the product window `p` at the hop, so the sum becomes 1.
      inline void normalize_synthesis(
         std::vector<double> const& p
       , std::vector<float>& synthesis
       , std::size_t hop)
      {
         auto const n = p.size();
         for (std::size_t i = 0; i != n; ++i)
         {
            double sum = 0;
            for (auto j = i % hop; j < n; j += hop)
               sum += p[j];
            synthesis[i] = sum > 0? float(synthesis[i] / sum) : 0.0f;
         }
      }
   }

   inline stft_window stft_window::hann(std::size_t n, std::size_t hop)
   {
      assert(hop > 0 && hop <= n);

      stft_window w;
      w.analysis.resize(n);
      w.synthesis.resize(n);
      std::vector<double> p(n);
      for (std::size_t i = 0; i != n; ++i)
      {
         auto h = detail::periodic_hann(i, n);
         w.analysis[i] = float(h);
         w.synthesis[i] = float(h);
         p[i] = h * h;
      }
      detail::normalize_synthesis(p, w.synthesis, hop);
      w.latency = n;
      return w;
   }

   inline stft_window stft_window::low_delay(
      std::size_t n
    , std::size_t hop
    , std::size_t delay)
   {
      assert(delay % 2 == 0 && delay >= 2 && delay <= n);
      assert(hop > 0 && hop <= delay);

      // The analysis window rises as the first half of a square-root Hann
      // of 2(n-m) samples, then falls as the second half of one of 2m. The
      // product is a Hann of 2m = delay samples, ending at the frame's end.
      auto const m = delay / 2;
      auto const start = n - delay;

      stft_window w;
      w.analysis.resize(n);
      w.synthesis.resize(n);
      std::vector<double> p(n);
      for (std::size_t i = 0; i != n; ++i)
      {
         double a = (i < n - m)?
            std::sqrt(detail::periodic_hann(i, 2 * (n - m))) :
            std::sqrt(detail::periodic_hann(i - start, delay));

         p[i] = (i < start)? 0.0 : detail::periodic_hann(i - start, delay);
         w.analysis[i] = float(a);
         w.synthesis[i] = (a > 0)? float(p[i] / a) : 0.0f;
      }
      detail::normalize_synthesis(p, w.synthesis, hop);
      w.latency = delay;
      return w;
   }

   inline std::size_t stft_window::size() const
   {
      return analysis.size();
   }
}

#endif
