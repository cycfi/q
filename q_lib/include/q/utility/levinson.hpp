/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LEVINSON_NOVEMBER_2_2026)
#define CYCFI_Q_LEVINSON_NOVEMBER_2_2026

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // levinson: the Levinson-Durbin recursion. From the autocorrelation r[0]
   // .. r[N] of a signal, the N reflection coefficients k of the all-pole
   // model whose spectrum matches those lags (linear prediction), its
   // prediction polynomial a, A(z) = a[0] + a[1] z^-1 + ... with a[0] = 1,
   // and the prediction error, returned. N is the size of k; r and a hold
   // N + 1. The k are as lattice_fir and lattice_iir take them. Each k is
   // held within +-0.9999, so the model stays stable. A silent r (r[0] not
   // positive) gives all k zero, A(z) = 1 and an error of 0.
   //
   // The recursion is in double: a spectral envelope's dynamic range can
   // exceed what float holds across N steps. a is updated in place, so
   // levinson needs no other storage.
   ////////////////////////////////////////////////////////////////////////////
   inline double levinson(
      std::span<double const> r
    , std::span<double> k
    , std::span<double> a
   )
   {
      auto const n = k.size();
      assert(r.size() == n + 1 && a.size() == n + 1);

      std::fill(k.begin(), k.end(), 0.0);
      std::fill(a.begin(), a.end(), 0.0);
      a[0] = 1.0;
      if (!(r[0] > 0.0))
         return 0.0;

      double err = r[0];
      for (std::size_t m = 1; m <= n; ++m)
      {
         double acc = r[m];
         for (std::size_t j = 1; j < m; ++j)
            acc += a[j] * r[m - j];
         auto const km = std::clamp(-acc / err, -0.9999, 0.9999);

         // a[j] += km a[m - j], in place: update both ends of each pair.
         for (std::size_t j = 1, i = m - 1; j <= i; ++j, --i)
         {
            auto const aj = a[j];
            auto const ai = a[i];
            a[j] = aj + km * ai;
            if (i != j)
               a[i] = ai + km * aj;
         }
         a[m] = km;
         k[m - 1] = km;
         err *= 1.0 - km * km;
      }
      return err;
   }
}

#endif
