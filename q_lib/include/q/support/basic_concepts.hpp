/*=============================================================================
   Copyright (c) 2014-2024 Joel de Guzman. All rights reserved.

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
   concept IndexableContainer = requires(T& x, std::size_t i)
   {
      { x[i] } -> std::convertible_to<typename T::value_type>;
      { x.size() } -> std::convertible_to<std::size_t>;
   };

   template <typename T>
   concept RandomAccessIteratable =
      std::random_access_iterator<typename T::iterator> &&
      requires(T& c)
   {
      { c.begin() } -> std::same_as<typename T::iterator>;
      { c.end() } -> std::same_as<typename T::iterator>;
   };
}

#endif

