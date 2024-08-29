/*=============================================================================
   Copyright (C) 2006 by Volodymyr Myrnyy (Vladimir Mirnyi)

   Permission to use, copy, modify, distribute and sell this software for any
   purpose is hereby granted without fee, provided that the above copyright
   notice appear in all copies and that both that copyright notice and this
   permission notice appear in supporting documentation.

   Generic simple and efficient Fast Fourier Transform (FFT) implementation
   using template metaprogramming

   https://www.eetimes.com/a-simple-and-efficient-fft-implementation-in-c-part-i/
   https://www.eetimes.com/a-simple-and-efficient-fft-implementation-in-c-part-ii/

   A new efficient implementation of the Cooley-Tukey fast Fourier transform
   (FFT) algorithm using C++ template metaprogramming. Thanks to the
   recursive nature of the FFT, the source code is more readable and faster
   than the classical implementation. The efficiency is proved by performance
   benchmarks on different platforms.
=============================================================================*/
#if !defined(CYCFI_Q_FFT_DECEMBER_25_2018)
#define CYCFI_Q_FFT_DECEMBER_25_2018

#include <q/support/literals.hpp>
#include <infra/support.hpp>
#include <utility>

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
      template <unsigned M, unsigned N, unsigned B, unsigned A>
      constexpr double sin_cos_series()
      {
         if constexpr (M == N)
            return 1.0;
         else
            return 1 - (A*pi/B)*(A*pi/B)/M/(M+1)
              *sin_cos_series<M+2, N, B, A>();
      }

      // Computes the sine of (A*pi/B) using the series expansion.
      //
      // B - Denominator for the angle in radians
      // A - Numerator for the angle in radians
      template <unsigned B, unsigned A>
      constexpr double sin()
      {
         return (A*pi/B)*sin_cos_series<2, 34, B, A>();
      }

      // Computes the cosine of (A*pi/B) using the series expansion.
      //
      // B - Denominator for the angle in radians
      // A - Numerator for the angle in radians
      template <unsigned B, unsigned A>
      constexpr double cos()
      {
         return sin_cos_series<1, 33, B, A>();
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
      template <std::size_t N>
      struct danielson_lanczos
      {
         static_assert(is_pow2(N), "N must be a power of 2");

         danielson_lanczos<N/2> next;

         void apply(double* data)
         {
            next.apply(data);
            next.apply(data+N);

            constexpr auto sina = -sin<N, 1>();
            constexpr auto sinb = -sin<N, 2>();

            double wtemp = sina;
            double wpr = -2.0*wtemp*wtemp;
            double wpi = sinb;
            double wr = 1.0;
            double wi = 0.0;
            for (std::size_t i=0; i<N; i+=2)
            {
               double tempr = data[i+N]*wr - data[i+N+1]*wi;
               double tempi = data[i+N]*wi+data[i+N+1]*wr;
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
      template <>
      struct danielson_lanczos<4>
      {
         void apply(double* data)
         {
            double tr = data[2];
            double ti = data[3];
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
      template <>
      struct danielson_lanczos<2>
      {
         void apply(double* data)
         {
            double tr = data[2];
            double ti = data[3];
            data[2] = data[0]-tr;
            data[3] = data[1]-ti;
            data[0] += tr;
            data[1] += ti;
         }
      };

         // The Danielson-Lanczos algorithm specialization for N=1.
      template <>
      struct danielson_lanczos<1>
      {
         void apply(double* data)
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
      template <std::size_t N>
      inline void scramble(double* data)
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

   template <std::size_t N>
   inline void fft(double* data)
   {
      detail::danielson_lanczos<N> recursion;
      detail::scramble<N>(data);
      recursion.apply(data);
   }

   template <std::size_t N>
   void ifft(double* data)
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
}

#endif
