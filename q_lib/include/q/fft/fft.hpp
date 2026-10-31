/*=============================================================================
   Copyright (c) 2006 by Volodymyr Myrnyy (Vladimir Mirnyi)
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Permission to use, copy, modify, distribute and sell this software for any
   purpose is hereby granted without fee, provided that the above copyright
   notice appear in all copies and that both that copyright notice and this
   permission notice appear in supporting documentation.

   Generic simple and efficient Fast Fourier Transform (FFT) implementation
   using template metaprogramming

   A new efficient implementation of the Cooley-Tukey fast Fourier transform
   (FFT) algorithm using C++ template metaprogramming. Thanks to the
   recursive nature of the FFT, the source code is more readable and faster
   than the classical implementation. The efficiency is proved by performance
   benchmarks on different platforms.

   References:

   https://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857
   https://gfft.sourceforge.net/
   https://www.eetimes.com/a-simple-and-efficient-fft-implementation-in-c-part-i/
   https://www.eetimes.com/a-simple-and-efficient-fft-implementation-in-c-part-ii/

=============================================================================*/
#if !defined(CYCFI_Q_FFT_DECEMBER_25_2018)
#define CYCFI_Q_FFT_DECEMBER_25_2018

#include <q/support/literals.hpp>
#include <infra/support.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <concepts>
#include <numbers>
#include <utility>

#if defined(Q_USE_PFFFT)
# include <q/fft/detail/pffft.hpp>
#endif

namespace cycfi::q
{
   namespace detail
   {
      // The sine and cosine values calculated using series expansion. These
      // functions are designed to be evaluated at compile time.
      //
      // M - Current term in the series expansion
      // N - Final term in the series expansion
      // B - Denominator for the angle in radians
      // A - Numerator for the angle in radians
      template <std::floating_point T, unsigned M, unsigned N, unsigned B, unsigned A>
      constexpr T sin_cos_series()
      {
         if constexpr (M == N)
         {
            return 1.0;
         }
         else
         {
            auto x = A*pi/B;
            return 1.0 - x*x/M/(M+1)*sin_cos_series<T, M+2, N, B, A>();
         }
      }

      constexpr auto sin_cos_accuracy = 40;

      // Computes the sine of (A*pi/B) using the series expansion.
      //
      // B - Denominator for the angle in radians
      // A - Numerator for the angle in radians
      template <std::floating_point T, unsigned B, unsigned A>
      constexpr T sin()
      {
         return (A*pi/B)*sin_cos_series<T, 2, sin_cos_accuracy, B, A>();
      }

      // Computes the cosine of (A*pi/B) using the series expansion.
      //
      // B - Denominator for the angle in radians
      // A - Numerator for the angle in radians
      template <std::floating_point T, unsigned B, unsigned A>
      constexpr T cos()
      {
         return sin_cos_series<T, 1, sin_cos_accuracy-1, B, A>();
      }

      // `danielson_lanczos` is a template struct that takes the size of the
      // input data as a template parameter, which must be a power of 2.
      //
      // The recursive nature of the Danielson-Lanczos algorithm is realized
      // by two recursive calls to the member function apply: the first call
      // processes the original signal data, and the second call processes
      // the data shifted by N. Each recursion level divides N by 2.
      //
      // The trigonometric functions are calculated using a series expansion,
      // using the sin<A, B>() function. All arguments are template
      // parameters, and the results are constants evaluated at compile time
      // at each level.
      //
      // The resulting (tempr, tempi) is a temporary complex number to modify
      // (data[i+N], data[i+N+1]) and (data[i], data[i+1]).
      //
      // Short length FFTs could use fewer operations than the general
      // algorithm. Such particular cases are incorporated using partial
      // specialization of the template class danielson_lanczos.
      // Specializations exist for input sizes of 1, 2 and 4.
      template <std::floating_point T, std::size_t N>
      struct danielson_lanczos
      {
         static_assert(is_pow2(N), "N must be a power of 2");

         danielson_lanczos<T, N/2> next;

         void apply(T* data)
         {
            next.apply(data);
            next.apply(data+N);

            constexpr auto sina = -sin<T, N, 1>();
            constexpr auto sinb = -sin<T, N, 2>();

            T wtemp = sina;
            T wpr = -2.0*wtemp*wtemp;
            T wpi = sinb;
            T wr = 1.0;
            T wi = 0.0;
            for (std::size_t i=0; i<N; i+=2)
            {
               T tempr = data[i+N]*wr - data[i+N+1]*wi;
               T tempi = data[i+N]*wi+data[i+N+1]*wr;
               data[i+N] = data[i]-tempr;
               data[i+N+1] = data[i+1]-tempi;
               data[i] += tempr;
               data[i+1] += tempi;

               wtemp = wr;
               wr += wr*wpr - wi*wpi;
               wi += wi*wpr+wtemp*wpi;
            }
         }
      };

      // The Danielson-Lanczos algorithm specialization for N=4.
      template <std::floating_point T>
      struct danielson_lanczos<T, 4>
      {
         void apply(T* data)
         {
            T tr = data[2];
            T ti = data[3];
            data[2] = data[0]-tr;
            data[3] = data[1]-ti;
            data[0] += tr;
            data[1] += ti;
            tr = data[6];
            ti = data[7];
            data[6] = data[5]-ti;
            data[7] = tr-data[4];
            data[4] += tr;
            data[5] += ti;

            tr = data[4];
            ti = data[5];
            data[4] = data[0]-tr;
            data[5] = data[1]-ti;
            data[0] += tr;
            data[1] += ti;
            tr = data[6];
            ti = data[7];
            data[6] = data[2]-tr;
            data[7] = data[3]-ti;
            data[2] += tr;
            data[3] += ti;
         }
      };

      // The Danielson-Lanczos algorithm specialization for N=2.
      template <std::floating_point T>
      struct danielson_lanczos<T, 2>
      {
         void apply(T* data)
         {
            T tr = data[2];
            T ti = data[3];
            data[2] = data[0]-tr;
            data[3] = data[1]-ti;
            data[0] += tr;
            data[1] += ti;
         }
      };

         // The Danielson-Lanczos algorithm specialization for N=1.
      template <std::floating_point T>
      struct danielson_lanczos<T, 1>
      {
         void apply(T* data)
         {
         }
      };

      // Scramble the data. The scrambling intended is reverse-binary
      // reindexing, meaning that x[j] gets replaced by x[k], where k is the
      // reverse-binary representation of j.
      //
      // The function iterates over the data array and swaps elements to
      // achieve the bit-reversed order. The bit-reversal is done by
      // incrementing the index j in a specific pattern that ensures the
      // correct order.
      //
      // This bit-reversal permutation allows the FFT to be computed in place.
      template <std::floating_point T, std::size_t N>
      inline void scramble(T* data)
      {
         int j=1;
         for (int i=1; i<2*N; i+=2)
         {
            if (j > i)
            {
               std::swap(data[j-1], data[i-1]);
               std::swap(data[j], data[i]);
            }
            int m = N;
            while (m >=2 && j > m)
            {
               j -= m;
               m >>= 1;
            }
            j += m;
         }
      }

      template <std::size_t N, std::floating_point T>
      inline void native_fft(T* data)
      {
         detail::danielson_lanczos<T, N> recursion;
         detail::scramble<T, N>(data);
         recursion.apply(data);
      }

      template <std::size_t N, std::floating_point T>
      inline void native_ifft(T* data)
      {
         constexpr std::size_t _2n = N*2;

         // Swap the real and imaginary parts of the i-th and (N-i)-th
         // complex numbers.
         for (std::size_t i = 1; i < N/2; ++i)
         {
            auto _2i = 2*i;
            std::swap(data[2*i], data[_2n-_2i]);
            std::swap(data[2*i+1], data[(_2n+1)-_2i]);
         }

         // Perform FFT in-situ
         native_fft<N>(data);

         // Normalize the data by dividing each element by N.
         for (std::size_t i = 0; i < 2*N; ++i)
            data[i] /= N;
      }

      // w[k] = exp(-2 pi i k / N), k = 0 .. N/4, for the real FFT's
      // split step. Computed once, in double, when the program loads.
      template <std::size_t N, std::floating_point T>
      inline std::array<std::complex<T>, N/4+1> const rfft_twiddles = []
      {
         std::array<std::complex<T>, N/4+1> w;
         for (std::size_t k = 0; k != w.size(); ++k)
         {
            double a = -2 * std::numbers::pi * k / N;
            w[k] = {T(std::cos(a)), T(std::sin(a))};
         }
         return w;
      }();

      // A real N-point FFT from a complex N/2-point one. The N samples,
      // read as N/2 complex values z[m] = x[2m] + i x[2m+1], transform to
      // Z. Bin pairs (k, N/2-k) of Z then split into the transforms of the
      // even and odd samples, E and O, and X[k] = E[k] + w[k] O[k].
      template <std::size_t N, std::floating_point T>
      inline void native_rfft(T* data)
      {
         constexpr std::size_t m = N/2;
         auto const& w = rfft_twiddles<N, T>;

         native_fft<m>(data);

         T r0 = data[0];
         T i0 = data[1];
         data[0] = r0 + i0;
         data[1] = 0;
         data[N] = r0 - i0;
         data[N+1] = 0;

         for (std::size_t k = 1; 2*k < m; ++k)
         {
            T* xk = data + 2*k;
            T* xj = data + 2*(m-k);

            T er = (xk[0] + xj[0]) / 2;
            T ei = (xk[1] - xj[1]) / 2;
            T or_ = (xk[1] + xj[1]) / 2;
            T oi = (xj[0] - xk[0]) / 2;
            T tr = w[k].real()*or_ - w[k].imag()*oi;
            T ti = w[k].real()*oi + w[k].imag()*or_;

            xk[0] = er + tr;
            xk[1] = ei + ti;
            xj[0] = er - tr;
            xj[1] = ti - ei;
         }

         // The middle bin pairs with itself: X[N/4] = conj(Z[N/4]).
         if constexpr (m >= 2)
            data[m+1] = -data[m+1];
      }

      // The inverse of native_rfft: rebuild Z from the bin pairs, then a
      // complex N/2-point inverse FFT gives the samples back.
      template <std::size_t N, std::floating_point T>
      inline void native_irfft(T* data)
      {
         constexpr std::size_t m = N/2;
         auto const& w = rfft_twiddles<N, T>;

         T e0 = (data[0] + data[N]) / 2;
         T o0 = (data[0] - data[N]) / 2;
         data[0] = e0;
         data[1] = o0;

         for (std::size_t k = 1; 2*k < m; ++k)
         {
            T* xk = data + 2*k;
            T* xj = data + 2*(m-k);

            T er = (xk[0] + xj[0]) / 2;
            T ei = (xk[1] - xj[1]) / 2;
            T tr = (xk[0] - xj[0]) / 2;
            T ti = (xk[1] + xj[1]) / 2;
            T or_ = w[k].real()*tr + w[k].imag()*ti;
            T oi = w[k].real()*ti - w[k].imag()*tr;

            xk[0] = er - oi;
            xk[1] = ei + or_;
            xj[0] = er + oi;
            xj[1] = or_ - ei;
         }

         if constexpr (m >= 2)
            data[m+1] = -data[m+1];

         native_ifft<m>(data);
      }

#if defined(Q_USE_PFFFT)
      // One PFFFT setup per size and kind, made when the program (or
      // plugin) loads, so a transform never allocates.
      template <std::size_t N, pffft::pffft_transform_t K>
      struct pffft_setup
      {
                              pffft_setup()
                               : _p{pffft::pffft_new_setup(N, K)}
                              {}

                              ~pffft_setup()
                              {
                                 pffft::pffft_destroy_setup(_p);
                              }

                              pffft_setup(pffft_setup const&) = delete;
         pffft_setup&         operator=(pffft_setup const&) = delete;

         pffft::PFFFT_Setup*  _p;
      };

      template <std::size_t N, pffft::pffft_transform_t K>
      inline pffft_setup<N, K> const pffft_setup_v;

      // PFFFT's smallest power-of-2 sizes. Smaller ones stay native.
      constexpr std::size_t pffft_min_complex = 16;
      constexpr std::size_t pffft_min_real = 32;

      // An ordered PFFFT transform in place. PFFFT's scratch lives on the
      // stack (2N floats complex, N real). A buffer that is misaligned for
      // PFFFT's SIMD goes through an aligned copy on the stack.
      template <std::size_t N, pffft::pffft_transform_t K>
      inline void pffft_run(float* data, pffft::pffft_direction_t dir)
      {
         constexpr auto n = (K == pffft::PFFFT_COMPLEX)? 2*N : N;
         auto setup = pffft_setup_v<N, K>._p;

         if (pffft::is_aligned(data))
         {
            pffft::pffft_transform_ordered(setup, data, data, nullptr, dir);
         }
         else
         {
            alignas(64) float tmp[n];
            std::copy(data, data+n, tmp);
            pffft::pffft_transform_ordered(setup, tmp, tmp, nullptr, dir);
            std::copy(tmp, tmp+n, data);
         }
      }

      template <std::size_t N>
      inline void scale(float* data, std::size_t n)
      {
         constexpr float s = 1.0f / N;
         for (std::size_t i = 0; i != n; ++i)
            data[i] *= s;
      }
#endif
   }
}

// With Q_USE_PFFFT, the float transforms run on PFFFT and everything else
// stays native. The two builds live in different inline namespaces, so
// translation units that disagree on Q_USE_PFFFT still link, each with its
// own implementation.
#if defined(Q_USE_PFFFT)
namespace cycfi::q::inline fft_pffft
#else
namespace cycfi::q::inline fft_native
#endif
{
   /**
    * \brief
    *    Performs an in-place Fast Fourier Transform (FFT) on the input data
    *    array.
    *
    *    Natively, the input is scrambled to bit-reversed order and the
    *    recursive Danielson-Lanczos algorithm computes the FFT. With
    *    Q_USE_PFFFT, float transforms of 16 points or more run on PFFFT.
    *
    * \tparam N
    *    The number of complex points. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to 2N elements: N complex numbers, interleaved real and
    *    imaginary parts.
    *
    * \note
    *    The input data array is modified in place. After the function
    *    returns, the array will contain the FFT of the original data.
   */
   template <std::size_t N, std::floating_point T>
   inline void fft(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float> && N >= detail::pffft_min_complex)
      {
         detail::pffft_run<N, detail::pffft::PFFFT_COMPLEX>(
            data, detail::pffft::PFFFT_FORWARD);
         return;
      }
#endif
      detail::native_fft<N>(data);
   }

   /**
    * \brief
    *    Performs an in-place Inverse Fast Fourier Transform (IFFT) on the
    *    input data array.
    *
    *    The result is normalized by 1/N, so `fft` followed by `ifft`
    *    restores the input.
    *
    * \tparam N
    *    The number of complex points. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to 2N elements: N complex numbers, interleaved real and
    *    imaginary parts.
    *
    * \note
    *    The input data array is modified in place. After the function
    *    returns, the array will contain the IFFT of the original data.
   */
   template <std::size_t N, std::floating_point T>
   inline void ifft(T* data)
   {
#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float> && N >= detail::pffft_min_complex)
      {
         detail::pffft_run<N, detail::pffft::PFFFT_COMPLEX>(
            data, detail::pffft::PFFFT_BACKWARD);
         detail::scale<N>(data, 2*N);
         return;
      }
#endif
      detail::native_ifft<N>(data);
   }

   /**
    * \brief
    *    Performs an in-place FFT of N real samples.
    *
    *    The result is the N/2+1 bins from DC through Nyquist, as
    *    interleaved complex numbers. The imaginary parts of the DC and
    *    Nyquist bins are zero. With Q_USE_PFFFT, float transforms of 32
    *    points or more run on PFFFT.
    *
    * \tparam N
    *    The number of real samples. Must be a power of 2, at least 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to N+2 elements. On input, the N samples occupy
    *    data[0] .. data[N-1]. On output, bin k is at (data[2k],
    *    data[2k+1]).
   */
   template <std::size_t N, std::floating_point T>
   inline void rfft(T* data)
   {
      static_assert(is_pow2(N) && N >= 2, "N must be a power of 2, >= 2");

#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float> && N >= detail::pffft_min_real)
      {
         detail::pffft_run<N, detail::pffft::PFFFT_REAL>(
            data, detail::pffft::PFFFT_FORWARD);

         // PFFFT packs the Nyquist bin into the imaginary slot of DC.
         data[N] = data[1];
         data[N+1] = 0;
         data[1] = 0;
         return;
      }
#endif
      detail::native_rfft<N>(data);
   }

   /**
    * \brief
    *    Performs an in-place inverse of `rfft`, from N/2+1 bins back to N
    *    real samples.
    *
    *    The result is normalized by 1/N, so `rfft` followed by `irfft`
    *    restores the input. The imaginary parts of the DC and Nyquist bins
    *    are ignored.
    *
    * \tparam N
    *    The number of real samples. Must be a power of 2, at least 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to N+2 elements. On input, bin k is at (data[2k],
    *    data[2k+1]). On output, the N samples occupy data[0] ..
    *    data[N-1].
   */
   template <std::size_t N, std::floating_point T>
   inline void irfft(T* data)
   {
      static_assert(is_pow2(N) && N >= 2, "N must be a power of 2, >= 2");

#if defined(Q_USE_PFFFT)
      if constexpr (std::same_as<T, float> && N >= detail::pffft_min_real)
      {
         data[1] = data[N];
         detail::pffft_run<N, detail::pffft::PFFFT_REAL>(
            data, detail::pffft::PFFFT_BACKWARD);
         detail::scale<N>(data, N);
         return;
      }
#endif
      detail::native_irfft<N>(data);
   }

   /**
    * \brief
    *    Computes the magnitude spectrum of a given data array using FFT.
    *
    *    This function performs an in-place Fast Fourier Transform (FFT) on
    *    the input data array and then computes the magnitude spectrum. The
    *    magnitude spectrum represents the amplitude of each frequency
    *    component in the input data.
    *
    * \tparam N
    *    The number of complex points. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to 2N elements: N complex numbers, interleaved real and
    *    imaginary parts.
    *
    * \note
    *    The input data array is modified in place. After the function
    *    returns, the array will contain the magnitude spectrum of the
    *    original data. The size of the magnitude spectrum is N/2+1.
   */
   template <std::size_t N, std::floating_point T>
   inline void magspec(T* data)
   {
      // Perform FFT in place
      fft<N>(data);

      // Compute the magnitude of each bin from DC through Nyquist. `fft`
      // stores bin k interleaved: real at data[2k], imaginary at data[2k+1].
      // Compacting into data[0..N/2] in ascending order is overwrite-safe:
      // iteration k reads indices >= 2k and writes index k, and every later
      // read starts at 2(k+1) = 2k+2 > k.
      for (std::size_t k = 0; k <= N/2; ++k)
         data[k] = std::hypot(data[2*k], data[2*k+1]);
   }
}

#endif
