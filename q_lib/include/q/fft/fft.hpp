/*=============================================================================
   Copyright (c) 2006 by Volodymyr Myrnyy (Vladimir Mirnyi)
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

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

#include <utility>
#include <array>
#include <concepts>

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
   }

   /**
    * \brief
    *    Performs an in-place Fast Fourier Transform (FFT) on the input data
    *    array using the Danielson-Lanczos algorithm.
    *
    *    The input data array is first scrambled to reorder the elements, and
    *    then the recursive Danielson-Lanczos algorithm is applied to compute
    *    the FFT.
    *
    * \tparam N
    *    The size of the input data array. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to the input data array. The array must have at least N
    *    elements.
    *
    * \note
    *    The input data array is modified in place. After the function
    *    returns, the array will contain the FFT of the original data.
   */
   template <std::size_t N, std::floating_point T>
   inline void fft(T* data)
   {
      detail::danielson_lanczos<T, N> recursion;
      detail::scramble<T, N>(data);
      recursion.apply(data);
   }

   /**
    * \brief
    *    Performs an in-place Inverse Fast Fourier Transform (IFFT) on the
    *    input data array.
    *
    *    This function performs an in-place IFFT on the input data array. It
    *    first swaps the real and imaginary parts of the i-th and (N-i)-th
    *    complex numbers, then performs an FFT, and finally normalizes the
    *    data by dividing each element by N.
    *
    * \tparam N
    *    The size of the input data array. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to the input data array. The array must have at least 2*N
    *    elements.
    *
    * \note
    *    The input data array is modified in place. After the function
    *    returns, the array will contain the IFFT of the original data.
   */
   template <std::size_t N, std::floating_point T>
   inline void ifft(T* data)
   {
      constexpr std::size_t _2n = N*2;
      // Swap the real and imaginary parts of the i-th and (N-i)-th complex
      // numbers.
      for (auto i = 1; i < N/2; ++i)
      {
         auto _2i = 2*i;
         std::swap(data[2*i], data[_2n-_2i]);
         std::swap(data[2*i+1], data[(_2n+1)-_2i]);
      }
      // Perform FFT in-situ
      fft<N>(data);
      // Normalize the data by dividing each element by N.
      for (auto i = 0; i < 2*N; ++i)
         data[i] /= N;
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
    *    The size of the input data array. Must be a power of 2.
    *
    * \tparam T
    *    The floating-point type of the input data (e.g., float, double).
    *
    * \param data
    *    A pointer to the input data array. The array must have at least N
    *    elements.
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

      // Compute the magnitude of the DC component (frequency 0)
      data[0] = std::abs(data[0]);

      for (std::size_t i = 1; i < N/2; ++i)
      {
         auto real = data[i];
         auto imag = data[N-i];
         data[i] = std::sqrt(real*real + imag*imag);
      }

      // Compute the magnitude of the Nyquist frequency component
      data[N/2] = std::abs(data[N/2]);
   }
}

#endif
