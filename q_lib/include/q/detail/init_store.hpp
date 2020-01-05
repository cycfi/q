/*=============================================================================
   Copyright (c) 2014-2019 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_INIT_STORE_JULY_12_2014)
#define CYCFI_Q_INIT_STORE_JULY_12_2014

#include <type_traits>

namespace cycfi::q::detail
{
   template <typename C, typename = int>
   struct resizable_container
    : std::false_type
   {};

   template <typename C>
   struct resizable_container<C, decltype(std::declval<C>().resize(1), 0)>
    : std::true_type
   {};

   template <typename T>
   void init_store(std::size_t size, std::vector<T>& _data, std::size_t& _mask)
   {
      // allocate the data in a size that is a power of two for efficient indexing
      std::size_t capacity = smallest_pow2(size);
      _mask = capacity - 1;
      _data.resize(capacity, T{});
   }

   template <typename T, std::size_t N>
   void init_store(std::array<T, N>& _data, std::size_t& _mask)
   {
      static_assert(is_pow2(N),
         "Error: Storage must have a size that is a power of two");
      _mask = _data.size() - 1;
   }
}

#endif
