/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_BITSTREAM_HPP_MARCH_12_2018)
#define CYCFI_Q_BITSTREAM_HPP_MARCH_12_2018

#include <type_traits>
#include <cstddef>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <q/support/base.hpp>

namespace cycfi::q
{
   ////////////////////////////////////////////////////////////////////////////
   // The bitset class stores bits efficiently using integers <T>. Data is
   // stored in a std::vector with a size that is fixed at construction time,
   // given the number of bits required.
   //
   // Member functions are provided for:
   //
   //    1. Setting individual bits and ranges of bits
   //    2. Geting each bit at position i
   //    3. Clearing all bits
   //    4. Getting the actual integers that stores the bits.
   ////////////////////////////////////////////////////////////////////////////
   template <typename T = natural_uint>
   class bitset
   {
   public:

      using value_type = T;
      using vector_type = std::vector<T>;

      static_assert(std::is_unsigned<T>::value, "T must be unsigned");
      static constexpr auto value_size = CHAR_BIT * sizeof(T);
      static constexpr auto one = T{1};

                     bitset(std::size_t num_bits);
                     bitset(bitset const& rhs) = default;
                     bitset(bitset&& rhs) = default;

      bitset&        operator=(bitset const& rhs) = default;
      bitset&        operator=(bitset&& rhs) = default;

      std::size_t    size() const;
      void           clear();
      void           set(std::size_t i, bool val);
      void           set(std::size_t i, std::size_t n, bool val);
      bool           get(std::size_t i) const;

      T*             data();
      T const*       data() const;

   private:

      vector_type    _bits;
   };

   ////////////////////////////////////////////////////////////////////////////
   // Implementation
   ////////////////////////////////////////////////////////////////////////////
   template <typename T>
   inline bitset<T>::bitset(std::size_t num_bits)
   {
      auto array_size = (num_bits + value_size - 1) / value_size;
      _bits.resize(array_size, 0);
   }

   template <typename T>
   inline std::size_t bitset<T>::size() const
   {
      return _bits.size() * value_size;
   }

   template <typename T>
   inline void bitset<T>::clear()
   {
      std::fill(_bits.begin(), _bits.end(), 0);
   }

   template <typename T>
   inline void bitset<T>::set(std::size_t i, bool val)
   {
      // Check that we don't get past the storage
      if (i > size())
         return;

      auto mask = 1 << (i % value_size);
      auto& ref = _bits[i / value_size];
      ref ^= (-T(val) ^ ref) & mask;
   }

   template <typename T>
   inline bool bitset<T>::get(std::size_t i) const
   {
      // Check we don't get past the storage
      if (i > size())
         return 0;

      auto mask = one << (i % value_size);
      return (_bits[i / value_size] & mask) != 0;
   }

   template <typename T>
   inline void bitset<T>::set(std::size_t i, std::size_t n, bool val)
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
         T mask = (one << mod) - 1;

         if (val)
            *p |= mask;
         else
            *p &= ~mask;
      }
   }

   template <typename T>
   inline T* bitset<T>::data()
   {
      return _bits.data();
   }

   template <typename T>
   inline T const* bitset<T>::data() const
   {
      return _bits.data();
   }
}

#endif

