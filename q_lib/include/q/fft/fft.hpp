/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FFT_DECEMBER_25_2018)
#define CYCFI_Q_FFT_DECEMBER_25_2018

#include <infra/support.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#if defined(Q_USE_PFFFT)
# include <q/fft/detail/pffft.hpp>
#endif

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // native_fft_plan: an iterative radix-2 FFT of n complex values,
   // interleaved, in place, its stages fused in pairs so each pass over the
   // data does two (a radix-4 pass in registers): the first pair, whose
   // twiddles are 1 and -i, with no multiplies. The twiddles are computed
   // once, in double, and the bit-reversal order is a table, both made at
   // construction.
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class native_fft_plan
   {
   public:

      explicit             native_fft_plan(std::size_t n);

      void                 forward(T* data) const;
      void                 inverse(T* data) const;
      void                 inverse_unscaled(T* data) const;

   private:

      template <bool Inverse>
      void                 transform(T* data) const;

      using twiddles = std::vector<std::complex<T>>;

      std::size_t                _n;
      twiddles                   _twiddles;
      std::vector<std::uint32_t> _swaps;  // bit-reversal pairs, i < j
   };

   ////////////////////////////////////////////////////////////////////////////
   // native_rfft_plan: a real FFT of n samples from a complex FFT of n/2.
   // The samples, read as n/2 complex values z[m] = x[2m] + i x[2m+1],
   // transform to Z. Bin pairs (k, n/2-k) of Z then split into the
   // transforms of the even and odd samples, E and O, and
   // X[k] = E[k] + w[k] O[k], w[k] = exp(-2 pi i k / n). The table holds
   // w[k] / 2, which takes the split's halving with it.
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class native_rfft_plan
   {
   public:

      explicit             native_rfft_plan(std::size_t n);

      void                 forward(T* data) const;
      void                 inverse(T* data) const;

   private:

      using twiddles = std::vector<std::complex<T>>;

      std::size_t          _n;
      native_fft_plan<T>   _half;
      twiddles             _w;
   };

#if defined(Q_USE_PFFFT)
   ////////////////////////////////////////////////////////////////////////////
   // pffft_plan: a PFFFT setup of n points, real or complex, with its own
   // aligned work buffer, and a buffer that a pointer misaligned for PFFFT's
   // SIMD goes through. Everything is allocated at construction.
   ////////////////////////////////////////////////////////////////////////////
   class pffft_plan
   {
   public:

      using kind_type = pffft::pffft_transform_t;
      using direction = pffft::pffft_direction_t;

                           pffft_plan(std::size_t n, kind_type kind);
                           pffft_plan(pffft_plan const& rhs);
                           pffft_plan(pffft_plan&& rhs) noexcept;
                           ~pffft_plan();

      pffft_plan&          operator=(pffft_plan rhs) noexcept;

      void                 run(float* data, direction dir);

   private:

      std::size_t          _n;
      kind_type            _kind;
      std::size_t          _scalars;
      pffft::PFFFT_Setup*  _setup = nullptr;
      float*               _work = nullptr;
      float*               _copy = nullptr;
   };

   // PFFFT's smallest power-of-2 sizes. Smaller ones stay native.
   constexpr std::size_t pffft_min_complex = 16;
   constexpr std::size_t pffft_min_real = 32;
#endif
}

// With Q_USE_PFFFT, float transforms run on PFFFT and everything else stays
// native. The two builds live in different inline namespaces, so
// translation units that disagree on Q_USE_PFFFT still link, each with its
// own implementation.
#if defined(Q_USE_PFFFT)
namespace cycfi::q::inline fft_pffft
#else
namespace cycfi::q::inline fft_native
#endif
{
   ////////////////////////////////////////////////////////////////////////////
   // basic_fft: an FFT of n complex values, interleaved real and imaginary
   // (2n scalars), in place. n is a power of 2, given at construction, which
   // is the only place it allocates. forward applies no scaling; inverse
   // divides by n, so forward then inverse restores the input. With
   // Q_USE_PFFFT, float transforms of 16 points or more run on PFFFT.
   //
   // An object holds scratch space, so two threads need two objects.
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class basic_fft
   {
   public:

      explicit             basic_fft(std::size_t n);

      std::size_t          size() const;
      void                 forward(T* data);
      void                 inverse(T* data);

   private:

      using native_plan = detail::native_fft_plan<T>;

      std::size_t                         _n;
      std::optional<native_plan>          _native;
#if defined(Q_USE_PFFFT)
      std::optional<detail::pffft_plan>   _pffft;
#endif
   };

   ////////////////////////////////////////////////////////////////////////////
   // basic_rfft: an FFT of n real samples, in place in n+2 scalars. forward
   // takes the samples in data[0] .. data[n-1] and gives the n/2+1 bins from
   // DC through Nyquist as interleaved complex values, bin k at (data[2k],
   // data[2k+1]); the imaginary parts of DC and Nyquist are zero. inverse
   // undoes it, ignoring those two, and divides by n. n is a power of 2, at
   // least 2. With Q_USE_PFFFT, float transforms of 32 samples or more run
   // on PFFFT.
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class basic_rfft
   {
   public:

      explicit             basic_rfft(std::size_t n);

      std::size_t          size() const;
      void                 forward(T* data);
      void                 inverse(T* data);

   private:

      using native_plan = detail::native_rfft_plan<T>;

      std::size_t                         _n;
      std::optional<native_plan>          _native;
#if defined(Q_USE_PFFFT)
      std::optional<detail::pffft_plan>   _pffft;
#endif
   };

   using fft = basic_fft<float>;
   using rfft = basic_rfft<float>;

   ////////////////////////////////////////////////////////////////////////////
   // Inlines
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   inline basic_fft<T>::basic_fft(std::size_t n)
    : _n{n}
   {
      assert(is_pow2(n) && n >= 1);
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (n >= detail::pffft_min_complex)
         {
            _pffft.emplace(n, detail::pffft::PFFFT_COMPLEX);
            return;
         }
      }
#endif
      _native.emplace(n);
   }

   template <std::floating_point T>
   inline std::size_t basic_fft<T>::size() const
   {
      return _n;
   }

   template <std::floating_point T>
   inline void basic_fft<T>::forward(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (_pffft)
         {
            _pffft->run(data, detail::pffft::PFFFT_FORWARD);
            return;
         }
      }
#endif
      _native->forward(data);
   }

   template <std::floating_point T>
   inline void basic_fft<T>::inverse(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (_pffft)
         {
            _pffft->run(data, detail::pffft::PFFFT_BACKWARD);
            float const s = 1.0f / _n;
            for (std::size_t i = 0; i != 2*_n; ++i)
               data[i] *= s;
            return;
         }
      }
#endif
      _native->inverse(data);
   }

   template <std::floating_point T>
   inline basic_rfft<T>::basic_rfft(std::size_t n)
    : _n{n}
   {
      assert(is_pow2(n) && n >= 2);
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (n >= detail::pffft_min_real)
         {
            _pffft.emplace(n, detail::pffft::PFFFT_REAL);
            return;
         }
      }
#endif
      _native.emplace(n);
   }

   template <std::floating_point T>
   inline std::size_t basic_rfft<T>::size() const
   {
      return _n;
   }

   template <std::floating_point T>
   inline void basic_rfft<T>::forward(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (_pffft)
         {
            _pffft->run(data, detail::pffft::PFFFT_FORWARD);

            // PFFFT packs the Nyquist bin into the imaginary slot of DC.
            data[_n] = data[1];
            data[_n + 1] = 0;
            data[1] = 0;
            return;
         }
      }
#endif
      _native->forward(data);
   }

   template <std::floating_point T>
   inline void basic_rfft<T>::inverse(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float>)
      {
         if (_pffft)
         {
            data[1] = data[_n];
            _pffft->run(data, detail::pffft::PFFFT_BACKWARD);
            float const s = 1.0f / _n;
            for (std::size_t i = 0; i != _n; ++i)
               data[i] *= s;
            return;
         }
      }
#endif
      _native->inverse(data);
   }
}

namespace cycfi::q::detail
{
   ////////////////////////////////////////////////////////////////////////////
   // native_fft_plan
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   inline native_fft_plan<T>::native_fft_plan(std::size_t n)
    : _n{n}
    , _twiddles(n / 2)
   {
      for (std::size_t k = 0; k != _twiddles.size(); ++k)
      {
         double a = -2 * std::numbers::pi * k / n;
         _twiddles[k] = {T(std::cos(a)), T(std::sin(a))};
      }

      std::size_t bits = 0;
      while ((std::size_t(1) << bits) < n)
         ++bits;
      for (std::size_t i = 0; i != n; ++i)
      {
         std::uint32_t r = 0;
         for (std::size_t b = 0; b != bits; ++b)
            r |= std::uint32_t((i >> b) & 1) << (bits - 1 - b);
         if (i < r)
         {
            _swaps.push_back(std::uint32_t(i));
            _swaps.push_back(r);
         }
      }
   }

   template <std::floating_point T>
   inline void native_fft_plan<T>::forward(T* data) const
   {
      transform<false>(data);
   }

   // The inverse without its 1/n, for a caller that scales on its own.
   template <std::floating_point T>
   inline void native_fft_plan<T>::inverse_unscaled(T* data) const
   {
      transform<true>(data);
   }

   template <std::floating_point T>
   inline void native_fft_plan<T>::inverse(T* data) const
   {
      transform<true>(data);
      T const s = T(1) / _n;
      for (std::size_t i = 0; i != 2*_n; ++i)
         data[i] *= s;
   }

   // The inverse is the forward transform with conjugate twiddles. A
   // radix-2 stage of length len pairs x[k] and x[k + len/2] in each block,
   // the second times w_len^k; two stages, len L then 2L, fuse into one
   // radix-4 pass over the four points a, b, c, d at k, k + L/2, k + L,
   // k + 3L/2: with w = w_2L^k, b takes w^2, c takes w and d w^3, and the
   // rest is sums and differences times -i (i for the inverse).
   template <std::floating_point T>
   template <bool Inverse>
   inline void native_fft_plan<T>::transform(T* data) const
   {
      auto const* sw = _swaps.data();
      for (std::size_t p = 0; p != _swaps.size(); p += 2)
      {
         auto const i = sw[p], j = sw[p + 1];
         std::swap(data[2*i], data[2*j]);
         std::swap(data[2*i + 1], data[2*j + 1]);
      }

      // a * w, w from the table, conjugated for the inverse.
      auto twiddle = [](T& r, T& i, std::complex<T> const& w)
      {
         T const wr = w.real();
         T const wi = Inverse ? -w.imag() : w.imag();
         T const tr = r * wr - i * wi;
         i = r * wi + i * wr;
         r = tr;
      };

      std::size_t len = 2;
      if (_n >= 4)
      {
         // Lengths 2 and 4: twiddles 1, then 1 and -i (i for the inverse).
         for (std::size_t i = 0; i != _n; i += 4)
         {
            T* x = data + 2*i;
            T const ar = x[0] + x[2], ai = x[1] + x[3];
            T const br = x[0] - x[2], bi = x[1] - x[3];
            T const cr = x[4] + x[6], ci = x[5] + x[7];
            T const dr = x[4] - x[6], di = x[5] - x[7];
            T const vr = Inverse ? -di : di;
            T const vi = Inverse ? dr : -dr;
            x[0] = ar + cr;
            x[1] = ai + ci;
            x[4] = ar - cr;
            x[5] = ai - ci;
            x[2] = br + vr;
            x[3] = bi + vi;
            x[6] = br - vr;
            x[7] = bi - vi;
         }
         len = 8;
      }

      for (; 2*len <= _n; len *= 4)
      {
         auto const q = len / 2;
         auto const s1 = _n / len;
         auto const s2 = s1 / 2;

         // Each k's three twiddles serve every block: load them once, and
         // sweep the blocks. w^3 = w_2L^3k is past the table's half
         // circle once 3k >= L: there it is -w_2L^(3k - L).
         for (std::size_t k = 0; k != q; ++k)
         {
            auto const w1 = _twiddles[k * s2];
            auto const w2 = _twiddles[k * s1];
            auto const k3 = 3 * k;
            auto w3 = k3 < len
               ? _twiddles[k3 * s2] : -_twiddles[(k3 - len) * s2];
            for (std::size_t i = k; i < _n; i += 2*len)
            {
               T* a = data + 2*i;
               T* b = a + 2*q;
               T* c = a + 2*len;
               T* d = c + 2*q;

               T br = b[0], bi = b[1];
               T cr = c[0], ci = c[1];
               T dr = d[0], di = d[1];
               twiddle(br, bi, w2);
               twiddle(cr, ci, w1);
               twiddle(dr, di, w3);
               T const s0r = a[0] + br, s0i = a[1] + bi;
               T const s1r = a[0] - br, s1i = a[1] - bi;
               T const s2r = cr + dr, s2i = ci + di;
               T const s3r = cr - dr, s3i = ci - di;

               // b and d: s1 -+ i s3 going forward, s1 +- i s3 back.
               T const jr = Inverse ? -s3i : s3i;
               T const ji = Inverse ? s3r : -s3r;
               a[0] = s0r + s2r;
               a[1] = s0i + s2i;
               c[0] = s0r - s2r;
               c[1] = s0i - s2i;
               b[0] = s1r + jr;
               b[1] = s1i + ji;
               d[0] = s1r - jr;
               d[1] = s1i - ji;
            }
         }
      }

      // An odd stage left over.
      if (len <= _n)
      {
         auto const half = len / 2;
         auto const step = _n / len;
         for (std::size_t i = 0; i != _n; i += len)
         {
            for (std::size_t k = 0; k != half; ++k)
            {
               T* u = data + 2*(i + k);
               T* v = data + 2*(i + k + half);
               T vr = v[0], vi = v[1];
               twiddle(vr, vi, _twiddles[k * step]);
               v[0] = u[0] - vr;
               v[1] = u[1] - vi;
               u[0] += vr;
               u[1] += vi;
            }
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   // native_rfft_plan
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   inline native_rfft_plan<T>::native_rfft_plan(std::size_t n)
    : _n{n}
    , _half{n / 2}
    , _w(n / 4 + 1)
   {
      for (std::size_t k = 0; k != _w.size(); ++k)
      {
         double a = -2 * std::numbers::pi * k / n;
         _w[k] = {T(0.5 * std::cos(a)), T(0.5 * std::sin(a))};
      }
   }

   template <std::floating_point T>
   inline void native_rfft_plan<T>::forward(T* data) const
   {
      auto const n = _n;
      auto const m = n / 2;

      _half.forward(data);

      T r0 = data[0];
      T i0 = data[1];
      data[0] = r0 + i0;
      data[1] = 0;
      data[n] = r0 - i0;
      data[n + 1] = 0;

      // 2E and 2O from the pair, then X = E + w O with w / 2 from the
      // table, the remaining halving taken in the sums.
      for (std::size_t k = 1; 2*k < m; ++k)
      {
         T* xk = data + 2*k;
         T* xj = data + 2*(m - k);

         T const er = xk[0] + xj[0];
         T const ei = xk[1] - xj[1];
         T const or_ = xk[1] + xj[1];
         T const oi = xj[0] - xk[0];
         T const hr = _w[k].real(), hi = _w[k].imag();
         T const tr = hr * or_ - hi * oi;
         T const ti = hr * oi + hi * or_;

         xk[0] = T(0.5) * er + tr;
         xk[1] = T(0.5) * ei + ti;
         xj[0] = T(0.5) * er - tr;
         xj[1] = ti - T(0.5) * ei;
      }

      // The middle bin pairs with itself: X[n/4] = conj(Z[n/4]).
      if (m >= 2)
         data[m + 1] = -data[m + 1];
   }

   template <std::floating_point T>
   inline void native_rfft_plan<T>::inverse(T* data) const
   {
      auto const n = _n;
      auto const m = n / 2;

      // The half transform's 1/m and this step's 1/2, together: 1/n.
      T const s = T(1) / T(n);
      T e0 = (data[0] + data[n]) * s;
      T o0 = (data[0] - data[n]) * s;
      data[0] = e0;
      data[1] = o0;

      // The table's w / 2 makes the twiddled terms want 2 s.
      T const s2 = 2 * s;
      for (std::size_t k = 1; 2*k < m; ++k)
      {
         T* xk = data + 2*k;
         T* xj = data + 2*(m - k);

         T const er = (xk[0] + xj[0]) * s;
         T const ei = (xk[1] - xj[1]) * s;
         T const tr = xk[0] - xj[0];
         T const ti = xk[1] + xj[1];
         T const hr = _w[k].real(), hi = _w[k].imag();
         T const or_ = (hr * tr + hi * ti) * s2;
         T const oi = (hr * ti - hi * tr) * s2;

         xk[0] = er - oi;
         xk[1] = ei + or_;
         xj[0] = er + oi;
         xj[1] = or_ - ei;
      }

      // The middle bin, X[n/4], pairs with itself: conjugated, at 1/m.
      if (m >= 2)
      {
         data[m] *= 2 * s;
         data[m + 1] = -data[m + 1] * (2 * s);
      }

      _half.inverse_unscaled(data);
   }

#if defined(Q_USE_PFFFT)
   ////////////////////////////////////////////////////////////////////////////
   // pffft_plan
   ////////////////////////////////////////////////////////////////////////////
   inline pffft_plan::pffft_plan(std::size_t n, kind_type kind)
    : _n{n}
    , _kind{kind}
    , _scalars{(kind == pffft::PFFFT_COMPLEX)? 2*n : n}
   {
      auto bytes = _scalars * sizeof(float);
      _setup = pffft::pffft_new_setup(int(n), kind);
      _work = static_cast<float*>(pffft::pffft_aligned_malloc(bytes));
      _copy = static_cast<float*>(pffft::pffft_aligned_malloc(bytes));
   }

   inline pffft_plan::pffft_plan(pffft_plan const& rhs)
    : pffft_plan(rhs._n, rhs._kind)
   {}

   inline pffft_plan::pffft_plan(pffft_plan&& rhs) noexcept
    : _n{rhs._n}
    , _kind{rhs._kind}
    , _scalars{rhs._scalars}
    , _setup{std::exchange(rhs._setup, nullptr)}
    , _work{std::exchange(rhs._work, nullptr)}
    , _copy{std::exchange(rhs._copy, nullptr)}
   {}

   inline pffft_plan::~pffft_plan()
   {
      if (_setup)
         pffft::pffft_destroy_setup(_setup);
      pffft::pffft_aligned_free(_work);
      pffft::pffft_aligned_free(_copy);
   }

   inline pffft_plan& pffft_plan::operator=(pffft_plan rhs) noexcept
   {
      std::swap(_n, rhs._n);
      std::swap(_kind, rhs._kind);
      std::swap(_scalars, rhs._scalars);
      std::swap(_setup, rhs._setup);
      std::swap(_work, rhs._work);
      std::swap(_copy, rhs._copy);
      return *this;
   }

   inline void pffft_plan::run(float* data, direction dir)
   {
      if (pffft::is_aligned(data))
      {
         pffft::pffft_transform_ordered(_setup, data, data, _work, dir);
      }
      else
      {
         std::copy(data, data + _scalars, _copy);
         pffft::pffft_transform_ordered(_setup, _copy, _copy, _work, dir);
         std::copy(_copy, _copy + _scalars, data);
      }
   }
#endif
}

#endif
