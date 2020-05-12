/*=============================================================================
   Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_FRACTIONAL_RING_BUFFER_JULY_22_2014)
#define CYCFI_Q_FRACTIONAL_RING_BUFFER_JULY_22_2014

#include <q/utility/interpolation.hpp>
#include <q/utility/ring_buffer.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // fractional_ring_buffer: a fractional ring buffer allows sub-sample
   // indexing using interpolation.
   ////////////////////////////////////////////////////////////////////////////
   template <
      typename T
    , typename Storage = std::vector<T>
    , typename Index = float
    , typename Interpolation = sample_interpolation::linear>
   class fractional_ring_buffer : public ring_buffer<T, Storage>
   {
   public:

      using value_type = T;
      using storage_type = Storage;
      using interpolation_type = Interpolation;
      using base_type = ring_buffer<T, Storage>;

      using ring_buffer<T, Storage>::ring_buffer;

      // get data (index can be fractional)
      T const operator[](Index index) const
      {
         interpolation_type interpolate;
         return interpolate(static_cast<base_type const&>(*this), index);
      }
   };
}

#endif
