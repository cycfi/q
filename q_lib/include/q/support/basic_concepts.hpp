/*=============================================================================
   Copyright (c) 2014-2023 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(CYCFI_Q_CONCEPTS_HPP_MAY_12_2023)
#define CYCFI_Q_CONCEPTS_HPP_MAY_12_2023

#include <concepts>
#include <iterator>

namespace cycfi::q::concepts
{
   template <typename T>
   concept Arithmetic = std::integral<T> || std::floating_point<T>;

   template <typename T>
   concept Indexable = requires(T& x, std::size_t i)
   {
      x[i] -> T::value_type;
      x.size() -> std::size_t;
   };

   template <typename T>
   concept RAIteratable =
      std::random_access_iterator<typename T::iterator> &&
      requires(T& c)
   {
      c.begin() -> T::iterator;
      c.end() -> T::iterator;
   };
}

#endif

