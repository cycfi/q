/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_LPC_FIT_NOVEMBER_2_2026)
#define CYCFI_Q_LPC_FIT_NOVEMBER_2_2026

#include <q/fft/fft.hpp>
#include <q/utility/levinson.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>
#include <vector>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // lpc_fit: the all-pole model (linear prediction) of a spectral
   // envelope. Given the spectral envelope in dB on the bins of an n-point
   // spectrum, DC to Nyquist (n / 2 + 1 of them, n a power of 2), it fits
   // the model of the order given at construction: its reflection
   // coefficients k, as lattice_fir and lattice_iir take them, and its
   // prediction error, returned. The model's spectral envelope is then
   // err / |A(f)|^2, up to the spectrum's scale; response() gives |A(f)|^2
   // at a bin or on all of them, and polynomial() gives A(z).
   //
   // The spectral envelope's power, transformed back, gives its
   // autocorrelation at the first order + 1 lags; levinson gives the
   // model, with white noise 60 dB under the power so a model of a
   // spectral envelope with deep notches stays well conditioned. It
   // allocates only at construction.
   //
   // Response chooses how |A(f)|^2 is computed: lpc_clenshaw evaluates it
   // at each bin asked for, with no memory beyond the fit's; lpc_fft
   // transforms A once after each fit, for a spectrum's worth of memory
   // and an FFT. The default is lpc_clenshaw on a microcontroller (a
   // Cortex-M), where memory is short and double arithmetic slow, and
   // lpc_fft elsewhere.
   ////////////////////////////////////////////////////////////////////////////
   class lpc_clenshaw;
   class lpc_fft;

   namespace detail
   {
#if defined(__ARM_ARCH_PROFILE) && (__ARM_ARCH_PROFILE == 'M')
      using default_lpc_response = lpc_clenshaw;
#else
      using default_lpc_response = lpc_fft;
#endif
   }

   template <typename Response = detail::default_lpc_response>
   class basic_lpc_fit
   {
   public:

      using response_type = Response;

      explicit             basic_lpc_fit(
                              std::size_t bins
                            , std::size_t order = 32
                           );

      double               operator()(
                              std::span<float const> db
                            , std::span<float> k
                           );

      double               response(std::size_t bin) const;
      void                 response(std::span<float> mag2) const;
      std::span<double const> polynomial() const;
      std::size_t          bins() const;
      std::size_t          order() const;

   private:

      std::size_t          _n;
      std::vector<float>   _cos;    // cos(2 pi i / n), i = 0 .. n - 1
      std::vector<float>   _w;      // the power at each bin, weighted
      std::vector<double>  _r;      // the lags, 0 .. order
      std::vector<double>  _k;      // the reflection coefficients
      std::vector<double>  _a;      // A(z)
      response_type        _response;
   };

   using lpc_fit = basic_lpc_fit<>;

   ////////////////////////////////////////////////////////////////////////////
   // lpc_clenshaw: |A(f)|^2 = c[0] + 2 sum c[l] cos(l w), c the
   // autocorrelation of A's coefficients: a Chebyshev series in cos(w),
   // summed at a bin by Clenshaw's recurrence, in double (in float the
   // levels a formant table derives from it move by up to 0.16 dB).
   ////////////////////////////////////////////////////////////////////////////
   class lpc_clenshaw
   {
   public:

                           lpc_clenshaw(std::size_t bins, std::size_t order);

      void                 prepare(
                              std::span<double const> a
                            , std::span<float const> cos
                           );
      double               operator()(std::size_t bin) const;

   private:

      std::vector<double>  _c;
      float const*         _cos = nullptr;
   };

   ////////////////////////////////////////////////////////////////////////////
   // lpc_fft: |A(f)|^2 on every bin, by one FFT of A's coefficients after
   // each fit.
   ////////////////////////////////////////////////////////////////////////////
   class lpc_fft
   {
   public:

                           lpc_fft(std::size_t bins, std::size_t order);

      void                 prepare(
                              std::span<double const> a
                            , std::span<float const> cos
                           );
      double               operator()(std::size_t bin) const;

   private:

      basic_rfft<float>    _fft;
      std::vector<float>   _work;   // the FFT, then |A|^2 on the bins
   };

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <typename Response>
   inline basic_lpc_fit<Response>::basic_lpc_fit(
      std::size_t bins, std::size_t order)
    : _n{2 * (bins - 1)}
    , _cos(_n)
    , _w(bins)
    , _r(order + 1)
    , _k(order)
    , _a(order + 1)
    , _response{bins, order}
   {
      assert(bins > 2 && (_n & (_n - 1)) == 0 && order > 0);
      auto const pi = std::numbers::pi;
      for (std::size_t i = 0; i != _n; ++i)
         _cos[i] = float(std::cos(2 * pi * double(i) / double(_n)));
   }

   // r[l] = sum over the bins of w[b] cos(2 pi b l / n), the inverse
   // transform at lag l, with w the power counted twice for the bins that
   // stand for a pair of conjugates. A lag at a time, its cosine index
   // stepping by l modulo n (a power of 2, so a mask), in four interleaved
   // sums: no memory traffic for an in-order core, and no single chain of
   // dependent additions for an out-of-order one.
   template <typename Response>
   inline double basic_lpc_fit<Response>::operator()(
      std::span<float const> db, std::span<float> k)
   {
      assert(db.size() == _w.size() && k.size() == _k.size());
      auto const last = _w.size() - 1;
      for (std::size_t b = 0; b <= last; ++b)
      {
         auto const p = std::exp2(db[b] * 0.33219281f);     // 10^(dB/10)
         _w[b] = (b == 0 || b == last) ? p : 2 * p;
      }

      auto const* w = _w.data();
      auto const* c = _cos.data();
      auto const mask = _n - 1;
      auto const quads = _w.size() / 4;
      for (std::size_t l = 0; l != _r.size(); ++l)
      {
         double s0 = 0, s1 = 0, s2 = 0, s3 = 0;
         std::size_t i = 0;
         auto const l4 = 4 * l;
         for (std::size_t q = 0; q != quads; ++q, i += l4)
         {
            auto const* wq = w + 4 * q;
            s0 += double(wq[0]) * c[i & mask];
            s1 += double(wq[1]) * c[(i + l) & mask];
            s2 += double(wq[2]) * c[(i + 2 * l) & mask];
            s3 += double(wq[3]) * c[(i + 3 * l) & mask];
         }
         for (std::size_t b = 4 * quads; b <= last; ++b)
            s0 += double(w[b]) * c[(b * l) & mask];
         _r[l] = (s0 + s1) + (s2 + s3);
      }
      _r[0] *= 1.0 + 1e-6;

      auto const err = levinson(_r, _k, _a);
      for (std::size_t m = 0; m != _k.size(); ++m)
         k[m] = float(_k[m]);
      _response.prepare(_a, _cos);
      return err;
   }

   template <typename Response>
   inline double basic_lpc_fit<Response>::response(std::size_t bin) const
   {
      assert(bin < _w.size());
      return _response(bin);
   }

   template <typename Response>
   inline void basic_lpc_fit<Response>::response(std::span<float> mag2) const
   {
      assert(mag2.size() == _w.size());
      for (std::size_t b = 0; b != mag2.size(); ++b)
         mag2[b] = float(_response(b));
   }

   template <typename Response>
   inline std::span<double const> basic_lpc_fit<Response>::polynomial() const
   {
      return _a;
   }

   template <typename Response>
   inline std::size_t basic_lpc_fit<Response>::bins() const
   {
      return _w.size();
   }

   template <typename Response>
   inline std::size_t basic_lpc_fit<Response>::order() const
   {
      return _k.size();
   }

   inline lpc_clenshaw::lpc_clenshaw(std::size_t /* bins */, std::size_t order)
    : _c(order + 1)
   {
   }

   // A's autocorrelation, doubled past lag 0.
   inline void lpc_clenshaw::prepare(
      std::span<double const> a, std::span<float const> cos)
   {
      for (std::size_t l = 0; l != a.size(); ++l)
      {
         double sum = 0;
         for (std::size_t j = 0; j + l != a.size(); ++j)
            sum += a[j] * a[j + l];
         _c[l] = l == 0 ? sum : 2 * sum;
      }
      _cos = cos.data();
   }

   inline double lpc_clenshaw::operator()(std::size_t bin) const
   {
      double const x = _cos[bin];
      auto const* c = _c.data();
      double b1 = 0, b2 = 0;
      for (std::size_t l = _c.size() - 1; l >= 1; --l)
      {
         auto const b0 = c[l] + 2 * x * b1 - b2;
         b2 = b1;
         b1 = b0;
      }
      return std::max(c[0] + x * b1 - b2, 0.0);
   }

   inline lpc_fft::lpc_fft(std::size_t bins, std::size_t /* order */)
    : _fft{2 * (bins - 1)}
    , _work(2 * bins, 0.0f)
   {
   }

   // |A|^2 into the first bins of the transform, in place: bin b reads
   // scalars 2b and 2b + 1, never yet overwritten.
   inline void lpc_fft::prepare(
      std::span<double const> a, std::span<float const> /* cos */)
   {
      std::fill(_work.begin(), _work.end(), 0.0f);
      for (std::size_t j = 0; j != a.size(); ++j)
         _work[j] = float(a[j]);
      _fft.forward(_work.data());
      auto const bins = _work.size() / 2;
      for (std::size_t b = 0; b != bins; ++b)
      {
         auto const re = _work[2 * b];
         auto const im = _work[2 * b + 1];
         _work[b] = re * re + im * im;
      }
   }

   inline double lpc_fft::operator()(std::size_t bin) const
   {
      return _work[bin];
   }
}

#endif
