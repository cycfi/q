/*=============================================================================
   Copyright (c) 2014-2018 Joel de Guzman. All rights reserved.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(CYCFI_Q_BITSTREAM_HPP_MARCH_12_2018)
#define CYCFI_Q_BITSTREAM_HPP_MARCH_12_2018

#include <type_traits>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <q/support.hpp>

namespace cycfi { namespace q
{
   template <typename T = std::uint32_t>
   class bitstream
   {
   public:

      using value_type = T;
      using vector_type = std::vector<T>;

      static_assert(std::is_unsigned<T>::value, "T must be unsigned");
      static constexpr auto value_size = 8 * sizeof(T);

                     bitstream(std::size_t num_bits);
                     bitstream(bitstream const& rhs) = default;
                     bitstream(bitstream && rhs) = default;

      bitstream&     operator=(bitstream const& rhs) = default;
      bitstream&     operator=(bitstream && rhs) = default;

      std::size_t    size() const;
      void           clear();
      void           set(std::size_t i, bool val);
      void           set(std::size_t i, std::size_t n, bool val);
      bool           get(std::size_t i) const;
      void           shift_half();

      T*             data();
      T const*       data() const;

   private:

      vector_type    _bits;
   };

   template <typename T>
   inline bitstream<T>::bitstream(std::size_t num_bits)
   {
      auto array_size = (num_bits / value_size) + 1;
      if (array_size % 2)
         ++array_size;
      _bits.resize(array_size, 0);
   }

   template <typename T>
   inline std::size_t bitstream<T>::size() const
   {
      return _bits.size() * value_size;
   }

   template <typename T>
   inline void bitstream<T>::clear()
   {
      std::fill(_bits.begin(), _bits.end(), 0);
   }

   template <typename T>
   inline void bitstream<T>::set(std::size_t i, bool val)
   {
      // Check we don't get past the buffer
      if (i > size())
         return;

      auto mask = 1 << (i % value_size);
      auto& ref = _bits[i / value_size];
      ref ^= (-T(val) ^ ref) & mask;
   }

   template <typename T>
   inline bool bitstream<T>::get(std::size_t i) const
   {
      // Check we don't get past the buffer
      if (i > size())
         return 0;

      auto mask = 1 << (i % value_size);
      return (_bits[i / value_size] & mask) != 0;
   }

   template <typename T>
   inline void bitstream<T>::set(std::size_t i, std::size_t n, bool val)
   {
      // Check that the index (i) does not get past size
      auto size_ = size();
      if (i > size_)
         return;

      // Check that the n does not get past the size
      if ((i+n) > size_)
         n = size_-i;

      constexpr auto all_ones = int_traits<T>::max;
      auto* p = _bits.data();
      p += i / value_size;    // Adjust the buffer pointer for the current index (i)

      // Do the first partial int
      auto mod = i & (value_size-1);
      if (mod)
      {
         // mask off the high n bits we want to set
         mod = value_size-mod;

         // Calculate the mask
         T mask = ~(all_ones >> mod);

         // Adjust the mask if we're not going to reach the end of this int
         if (n < mod)
            mask &= (all_ones >> (mod-n));

         if (val)
            *p |= mask;
         else
            *p &= ~mask;

         // Fast exit if we're done here!
         if (n < mod)
            return;

         n -= mod;
         ++p;
      }

      // Write full ints while we can - effectively doing value_size bits at a time
      if (n >= value_size)
      {
         // Store a local value to work with
         T val_ = val ? all_ones : 0;

         do
         {
            *p++ = val_;
            n -= value_size;
         }
         while (n >= value_size);
      }

      // Now do the final partial int, if necessary
      if (n)
      {
         mod = n & (value_size-1);

         // Calculate the mask
         T mask = (1 << mod) - 1;

         if (val)
            *p |= mask;
         else
            *p &= ~mask;
      }
   }

   template <typename T>
   inline void bitstream<T>::shift_half()
   {
      std::copy(_bits.begin() + (_bits.size() / 2), _bits.end(), _bits.begin());
   }

   template <typename T>
   inline T* bitstream<T>::data()
   {
      return _bits.data();
   }

   template <typename T>
   inline T const* bitstream<T>::data() const
   {
      return _bits.data();
   }
}}

#endif

