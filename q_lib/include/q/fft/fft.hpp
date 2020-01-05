/*=============================================================================
   Copyright (C) 2006 by Volodymyr Myrnyy (Vladimir Mirnyi)

   Permission to use, copy, modify, distribute and sell this software for any
   purpose is hereby granted without fee, provided that the above copyright
   notice appear in all copies and that both that copyright notice and this
   permission notice appear in supporting documentation.

   Generic simple and efficient Fast Fourier Transform (FFT) implementation
   using template metaprogramming

   http://www.drdobbs.com/cpp/a-simple-and-efficient-fft-implementatio/199500857

   A new efficient implementation of the Cooley-Tukey fast Fourier transform
   (FFT) algorithm using C++ template metaprogramming. Thanks to the
   recursive nature of the FFT, the source code is more readable and faster
   than the classical implementation. The efficiency is proved by performance
   benchmarks on different platforms.
=============================================================================*/
#if !defined(CYCFI_Q_FFT_DECEMBER_25_2018)
#define CYCFI_Q_FFT_DECEMBER_25_2018

#include <q/support/literals.hpp>
#include <utility>

namespace cycfi::q
{
   namespace detail
   {
      // The sine and cosine values calculated using series expansion
      constexpr double sin_cos_series(unsigned M, unsigned N, unsigned B, unsigned A)
      {
         return (M == N) ? 1.0 :
            1-(A*pi/B)*(A*pi/B)/M/(M+1) * sin_cos_series(M+2, N, B, A);
      };

      constexpr double sin(unsigned B, unsigned A)
      {
         return (A*pi/B) * sin_cos_series(2, 34, B, A);
      }

      constexpr double cos(unsigned B, unsigned A)
      {
         return sin_cos_series(1, 33, B, A);
      }

      template <std::size_t N>
      struct danielson_lanczos
      {
         danielson_lanczos<N/2> next;

         void apply(double* data)
         {
            next.apply(data);
            next.apply(data+N);

            constexpr auto sina = -sin(N, 1);
            constexpr auto sinb = -sin(N, 2);

            double wtemp = sina;
            double wpr = -2.0*wtemp*wtemp;
            double wpi = sinb;
            double wr = 1.0;
            double wi = 0.0;
            for (std::size_t i=0; i<N; i+=2)
            {
               double tempr = data[i+N]*wr - data[i+N+1]*wi;
               double tempi = data[i+N]*wi + data[i+N+1]*wr;
               data[i+N] = data[i]-tempr;
               data[i+N+1] = data[i+1]-tempi;
               data[i] += tempr;
               data[i+1] += tempi;

               wtemp = wr;
               wr += wr*wpr - wi*wpi;
               wi += wi*wpr + wtemp*wpi;
            }
         }
      };

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
}

#endif
