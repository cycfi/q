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
   // interleaved, in place. The twiddles are computed once, in double, and
   // the bit-reversal order is a table, both made at construction.
   ////////////////////////////////////////////////////////////////////////////
   template <std::floating_point T>
   class native_fft_plan
   {
   public:

      explicit             native_fft_plan(std::size_t n);

      void                 forward(T* data) const;
      void                 inverse(T* data) const;

   private:

      void                 transform(T* data, T sign) const;

      using twiddles = std::vector<std::complex<T>>;

      std::size_t                _n;
      twiddles                   _twiddles;
      std::vector<std::uint32_t> _bitrev;
   };

   ////////////////////////////////////////////////////////////////////////////
   // native_rfft_plan: a real FFT of n samples from a complex FFT of n/2.
   // The samples, read as n/2 complex values z[m] = x[2m] + i x[2m+1],
   // transform to Z. Bin pairs (k, n/2-k) of Z then split into the
   // transforms of the even and odd samples, E and O, and
   // X[k] = E[k] + w[k] O[k], w[k] = exp(-2 pi i k / n).
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
    , _bitrev(n)
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
         _bitrev[i] = r;
      }
   }

   template <std::floating_point T>
   inline void native_fft_plan<T>::forward(T* data) const
   {
      transform(data, T(1));
   }

   template <std::floating_point T>
   inline void native_fft_plan<T>::inverse(T* data) const
   {
      transform(data, T(-1));
      T const s = T(1) / _n;
      for (std::size_t i = 0; i != 2*_n; ++i)
         data[i] *= s;
   }

   // The inverse is the forward transform with conjugate twiddles: `sign`
   // is -1 for it, flipping the twiddles' imaginary parts.
   template <std::floating_point T>
   inline void native_fft_plan<T>::transform(T* data, T sign) const
   {
      for (std::size_t i = 0; i != _n; ++i)
      {
         std::size_t j = _bitrev[i];
         if (i < j)
         {
            std::swap(data[2*i], data[2*j]);
            std::swap(data[2*i + 1], data[2*j + 1]);
         }
      }

      for (std::size_t len = 2; len <= _n; len *= 2)
      {
         auto const half = len / 2;
         auto const step = _n / len;
         for (std::size_t i = 0; i != _n; i += len)
         {
            for (std::size_t k = 0; k != half; ++k)
            {
               auto const& w = _twiddles[k * step];
               T wr = w.real();
               T wi = sign * w.imag();

               T* u = data + 2*(i + k);
               T* v = data + 2*(i + k + half);
               T vr = v[0] * wr - v[1] * wi;
               T vi = v[0] * wi + v[1] * wr;

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
         _w[k] = {T(std::cos(a)), T(std::sin(a))};
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

      for (std::size_t k = 1; 2*k < m; ++k)
      {
         T* xk = data + 2*k;
         T* xj = data + 2*(m - k);

         T er = (xk[0] + xj[0]) / 2;
         T ei = (xk[1] - xj[1]) / 2;
         T or_ = (xk[1] + xj[1]) / 2;
         T oi = (xj[0] - xk[0]) / 2;
         T tr = _w[k].real() * or_ - _w[k].imag() * oi;
         T ti = _w[k].real() * oi + _w[k].imag() * or_;

         xk[0] = er + tr;
         xk[1] = ei + ti;
         xj[0] = er - tr;
         xj[1] = ti - ei;
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

      T e0 = (data[0] + data[n]) / 2;
      T o0 = (data[0] - data[n]) / 2;
      data[0] = e0;
      data[1] = o0;

      for (std::size_t k = 1; 2*k < m; ++k)
      {
         T* xk = data + 2*k;
         T* xj = data + 2*(m - k);

         T er = (xk[0] + xj[0]) / 2;
         T ei = (xk[1] - xj[1]) / 2;
         T tr = (xk[0] - xj[0]) / 2;
         T ti = (xk[1] + xj[1]) / 2;
         T or_ = _w[k].real() * tr + _w[k].imag() * ti;
         T oi = _w[k].real() * ti - _w[k].imag() * tr;

         xk[0] = er - oi;
         xk[1] = ei + or_;
         xj[0] = er + oi;
         xj[1] = or_ - ei;
      }

      if (m >= 2)
         data[m + 1] = -data[m + 1];

      _half.inverse(data);
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
