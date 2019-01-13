/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018)
#define CYCFI_Q_AUTO_CORRELATOR_HPP_MARCH_12_2018

#include <q/utility/bitset.hpp>
#include <q/detail/count_bits.hpp>

namespace cycfi { namespace q
{
   struct auto_correlator
   {
      static constexpr auto value_size = bitset<>::value_size;

      auto_correlator(bitset<> const& bits)
         : _bits(bits)
         , _size(bits.size())
         , _mid_array(((_size / value_size) / 2) - 1)
      {}

      std::size_t operator()(std::size_t pos)
      {
         auto const index = pos / value_size;
         auto const shift = pos % value_size;

         auto const* p1 = _bits.data();
         auto const* p2 = _bits.data() + index;
         auto count = 0;

         if (shift == 0)
         {
            for (auto i = 0; i != _mid_array; ++i)
               count += detail::count_bits(*p1++ ^ *p2++);
         }
         else
         {
            auto shift2 = value_size - shift;
            for (auto i = 0; i != _mid_array; ++i)
            {
               auto v = *p2++ >> shift;
               v |= *p2 << shift2;
               count += detail::count_bits(*p1++ ^ v);
            }
         }
         return count;
      };

      bitset<> const&      _bits;
      std::size_t const    _size;
      std::size_t const    _mid_array;
   };
}}

#endif

