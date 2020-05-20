/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_MEDIAN_DECEMBER_7_2018)
#define CYCFI_Q_MEDIAN_DECEMBER_7_2018

#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // 3-point 1D median filter. Returns the median of 3 latest samples. The
   // median filter is a nonlinear digital filter often used to remove noise
   // from signals. The median filter performs better than a moving average
   // filter in reducing noise, especially outliers (sudden jumps in the
   // signal (impulsive noise)). The median filter, however, requires more
   // processing cycles.
   ////////////////////////////////////////////////////////////////////////////
   struct median3
   {
      median3(float median_ = 0.0f)
       : _median(median_)
      {}

      float operator()(float a)
      {
         _median = std::max(std::min(a, b), std::min(std::max(a, b), c));
         c = b;
         b = a;
         return _median;
      }

      float operator()() const
      {
         return _median;
      }

      median3& operator=(float median_)
      {
         _median = median_;
         return *this;
      }

      float _median = 0.0f;
      float b = 0.0f;
      float c = 0.0f;
   };
}

#endif
