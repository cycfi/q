/*=============================================================================
   Copyright (c) 2014-2026 Joel de Guzman. All rights reserved.

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#define CATCH_CONFIG_MAIN
#include <infra/catch.hpp>
#include <q/support/unit.hpp>

#include <type_traits>

namespace q = cycfi::q;

struct test_unit_tag;

template <typename T>
struct test_unit : q::unit<T, test_unit<T>>
{
   using base_type = q::unit<T, test_unit<T>>;
   using base_type::base_type;
   using unit_type = test_unit_tag;
};

using char_unit = test_unit<signed char>;
using short_unit = test_unit<short>;
using double_unit = test_unit<double>;
using int_unit = test_unit<int>;

TEST_CASE("Test_promote_unit")
{
   // The sum's type is one of the operands' types: that operand wins
   CHECK(std::is_same_v<q::promote_unit<double_unit, int_unit>, double_unit>);
   CHECK(std::is_same_v<q::promote_unit<int_unit, double_unit>, double_unit>);

   // Both are promoted (signed char + short is int): the larger one wins,
   // whichever side it is on
   CHECK(std::is_same_v<q::promote_unit<char_unit, short_unit>, short_unit>);
   CHECK(std::is_same_v<q::promote_unit<short_unit, char_unit>, short_unit>);

   // Equal sizes keep the left operand
   CHECK(std::is_same_v<q::promote_unit<char_unit, char_unit>, char_unit>);
}
